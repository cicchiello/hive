package com.jfc.apps.hive;

import java.util.Calendar;
import java.util.Locale;

import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.text.format.DateFormat;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;

import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.DbCredentialsProperty;
import com.jfc.misc.prop.StepsPerRevolutionProperty;
import com.jfc.misc.prop.ThreadsPerMeterProperty;
import com.jfc.srvc.cloud.CouchGetBackground;
import com.jfc.srvc.cloud.CouchPostBackground;
import com.jfc.srvc.cloud.CouchPutBackground;
import com.jfc.util.misc.SplashyText;

public class MotorProperty {
	private static final String TAG = MotorProperty.class.getName();

	private static final String MOTOR_ACTION_PROPERTY = "MOTOR_ACTION_PROPERTY";
	private static final String MOTOR_TIMESTAMP_PROPERTY = "MOTOR_TIMESTAMP_PROPERTY";
	private static final String DEFAULT_MOTOR_ACTION = "stopped";
	private static final String DEFAULT_MOTOR_TIMESTAMP = "0";
	
	private AlertDialog alert;
	private Activity activity;
	private TextView value, timestamp;
	private int motorIndex;
	private String hiveId;
	
	public static double stepsToLinearDistance(Activity activity, long steps) {
		double lsteps = steps;
		double threadsPerMeter = ThreadsPerMeterProperty.getThreadsPerMeter(activity);
		double stepsPerThread = StepsPerRevolutionProperty.getStepsPerRevolution(activity);
		return lsteps/stepsPerThread/threadsPerMeter;
	}

	public static long linearDistanceToSteps(Activity activity, double distanceMeters) {
		double threadsPerMeter = ThreadsPerMeterProperty.getThreadsPerMeter(activity);
		double stepsPerThread = StepsPerRevolutionProperty.getStepsPerRevolution(activity);
		double steps = distanceMeters*threadsPerMeter*stepsPerThread;
		return (long) (steps+0.5);
	}
	
	public MotorProperty(Activity _activity, String _hiveId, int _motorIndex, TextView _value, ImageButton button, TextView _timestamp) {
		this.activity = _activity;
		this.value = _value;
		this.timestamp = _timestamp;
		this.motorIndex = _motorIndex;
		this.hiveId = _hiveId;

		if (!isMotorPropertyDefined(activity, hiveId, motorIndex)) {
			setUndefined();
		} else {
			display(getMotorAction(activity, hiveId, motorIndex), getMotorTimestamp(activity, hiveId, motorIndex));
		}
		
    	button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				AlertDialog.Builder builder = new AlertDialog.Builder(activity);
				
				builder.setIcon(R.drawable.ic_hive);
				builder.setView(R.layout.motor_dialog);
				builder.setTitle("Motor "+Integer.toString(motorIndex+1)+" distance (mm)");
				builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
		        	@Override
		        	public void onClick(DialogInterface dialog, int which) {
						EditText et = (EditText) alert.findViewById(R.id.textValue);
						String linearDistanceMillimetersStr = et.getText().toString();
						double linearDistanceMillimeters = Long.parseLong(linearDistanceMillimetersStr);
						long steps = linearDistanceToSteps(activity, linearDistanceMillimeters/1000.0);

						if (steps != 0) {
							postToDb(Long.toString(steps));
							String action = "moving"+(steps > 0 ? "+":"-")+Long.toString(steps);
							long timestamp_s = System.currentTimeMillis()/1000;
							String timestamp = Long.toString(timestamp_s);
							MotorProperty.setMotorProperty(activity, hiveId, motorIndex, action, timestamp);
							display(action, timestamp_s);
						}
						
		        		alert.dismiss(); 
		        		alert = null;
		        	}
		        });
		        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {alert.dismiss(); alert = null;}
		        });
		        alert = builder.show();
		        
		        EditText tv = (EditText) alert.findViewById(R.id.textValue);
		        tv.setText("0");

		        alert.findViewById(R.id.plus).setOnClickListener(new View.OnClickListener() {public void onClick(View v) {inc();}});
		        alert.findViewById(R.id.plusPlus).setOnClickListener(new View.OnClickListener() {public void onClick(View v) {inc100();}});
		        alert.findViewById(R.id.minus).setOnClickListener(new View.OnClickListener() {public void onClick(View v) {dec();}});
		        alert.findViewById(R.id.minusMinus).setOnClickListener(new View.OnClickListener() {public void onClick(View v) {dec100();}});

			}
		});
	}

	public AlertDialog getAlertDialog() {return alert;}

	private void inc() {
		EditText tv = (EditText) getAlertDialog().findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)+1;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(activity, tv);
	}
	
	private void inc100() {
		EditText tv = (EditText) getAlertDialog().findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)+100;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(activity, tv);
	}
	
	private void dec() {
		EditText tv = (EditText) getAlertDialog().findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)-1;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(activity, tv);
	}
	
	private void dec100() {
		EditText tv = (EditText) getAlertDialog().findViewById(R.id.textValue);
		String valueStr = tv.getText().toString();
		int n = Integer.parseInt(valueStr)-100;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(activity, tv);
	}

	private void createNewMsgDoc(final String channelDocId, final String channelDocRev, String instruction, String prevId) {
		try {
			// create a new msg doc
			final String dbUrl = DbCredentialsProperty.getCouchChannelDbUrl(activity);
			final String authToken = DbCredentialsProperty.getAuthToken(activity);
			final String sensor = "motor"+Integer.toString(motorIndex)+"-target";
			String timestamp = Long.toString(System.currentTimeMillis()/1000);
			String payload = sensor + "|" + instruction;
	
			JSONObject msgDoc = new JSONObject();
			msgDoc.put("prev-msg-id", prevId);
			msgDoc.put("payload", payload);
			msgDoc.put("timestamp", timestamp);
			
		    CouchPostBackground.OnCompletion postOnCompletion = new CouchPostBackground.OnCompletion() {
		    	public void onSuccess(String msgId, String msgRev) {
		    		try {
		    			String timestamp = Long.toString(System.currentTimeMillis()/1000);
					
						JSONObject newChannelDoc = new JSONObject();
						if (channelDocRev != null)
							newChannelDoc.put("_rev", channelDocRev);
						newChannelDoc.put("msg-id", msgId);
						newChannelDoc.put("timestamp", timestamp);
		
						CouchPutBackground.OnCompletion putOnCompletion = new CouchPutBackground.OnCompletion() {
					    	public void complete(JSONObject results) {
					    		Log.i(TAG, "Channel Doc PUT success:  "+results.toString());
					    	}
					    	public void failed(String msg) {
					    		Log.e(TAG, "Channel Doc PUT failed: "+msg);
					    	}
						};
						
			    	    new CouchPutBackground(dbUrl+"/"+channelDocId, authToken, newChannelDoc.toString(), putOnCompletion).execute();
					} catch (JSONException je) {
						Log.e(TAG, je.getMessage());
					}
		    	}
		    	public void onFailure(String msg) {
		    		Log.e(TAG, "Msg Doc POST failed: "+msg);
		    	}
		    };
		    new CouchPostBackground(dbUrl, authToken, msgDoc.toString(), postOnCompletion).execute();
		} catch (JSONException je) {
			Log.e(TAG, je.getMessage());
		}
	}
	
	
	private void postToDb(final String instruction) {
		String ActiveHive = ActiveHiveProperty.getActiveHiveName(activity);
		String HiveId = HiveEnv.getHiveAddress(activity, ActiveHive);
		final String dbUrl = DbCredentialsProperty.getCouchChannelDbUrl(activity);
		String authToken = null;
		final String channelDocId = HiveId + "-app";
		
	    CouchGetBackground.OnCompletion channelDocOnCompletion = new CouchGetBackground.OnCompletion() {
			@Override
	    	public void complete(JSONObject currentChannelDoc) {
				try {
					String prevMsgId = currentChannelDoc.getString("msg-id");
					final String currentChannelDocRev = currentChannelDoc.getString("_rev");
					
					createNewMsgDoc(channelDocId, currentChannelDocRev, instruction, prevMsgId);
				} catch (JSONException je) {
					Log.e(TAG, je.getMessage());
				}
			}
			
			@Override
	    	public void failed(String msg) {
				// probably first time -- so create it
				Log.i(TAG, "Channel Doc GET failed: "+msg);
				
				createNewMsgDoc(channelDocId, null, instruction, "0");
			}
	    };

    	final CouchGetBackground getter = new CouchGetBackground(dbUrl+"/"+channelDocId, authToken, channelDocOnCompletion);
    	getter.execute();
	}
	
	private void setUndefined() {
		Log.i(TAG, "Set display undefined");
		long timestampMillis = 0;
		String action = "stopped";
		HiveEnv.setValueWithSplash(activity, value.getId(), timestamp.getId(), action, true, timestampMillis);
	}
	
	private void display(String actionStr, long timestampSeconds) {
		long timestampMillis = timestampSeconds*1000;
		String action = actionStr.equals("stopped") ? actionStr : "moving";
		HiveEnv.setValueWithSplash(activity, value.getId(), timestamp.getId(), action, true, timestampMillis);
	}

	
	private static String uniqueIdentifier(String base, String hiveId, int motorIndex) {
		return base+"|"+hiveId+"|"+motorIndex;
	}

	
	public static boolean isMotorPropertyDefined(Activity activity, String hiveId, int motorIndex) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		return  (SP.contains(uniqueIdentifier(MOTOR_ACTION_PROPERTY, hiveId, motorIndex)) && 
				 !SP.getString(uniqueIdentifier(MOTOR_ACTION_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_ACTION).equals(DEFAULT_MOTOR_ACTION)) ||
				(SP.contains(uniqueIdentifier(MOTOR_TIMESTAMP_PROPERTY, hiveId, motorIndex)) &&
				 !SP.getString(uniqueIdentifier(MOTOR_TIMESTAMP_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_TIMESTAMP).equals(DEFAULT_MOTOR_TIMESTAMP));
	}
	
	
	public static long getMotorTimestamp(Activity activity, String hiveId, int motorIndex) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(uniqueIdentifier(MOTOR_TIMESTAMP_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_TIMESTAMP);
		try {
			return Long.parseLong(v);
		} catch (NumberFormatException nfe) {
			return 0;
		}
	}
	
    public static String getMotorAction(Activity activity, String hiveId, int motorIndex) {
    	SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
    	return SP.getString(uniqueIdentifier(MOTOR_ACTION_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_ACTION);
    }
    
	public static void resetMotorProperty(Activity activity, String hiveId, int motorIndex) {
		setMotorProperty(activity, hiveId, motorIndex, DEFAULT_MOTOR_ACTION, DEFAULT_MOTOR_TIMESTAMP);
	}
	
	public static void setMotorProperty(Activity activity, String hiveId, int motorIndex, String action, long date) {
		setMotorProperty(activity, hiveId, motorIndex, action, date==0 ? DEFAULT_MOTOR_TIMESTAMP : Long.toString(date));
	}
	
	private static void setMotorProperty(Activity activity, String hiveId, int motorIndex, String action, String date) {
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(uniqueIdentifier(MOTOR_ACTION_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_ACTION).equals(action)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(uniqueIdentifier(MOTOR_ACTION_PROPERTY, hiveId, motorIndex), action);
				editor.commit();
			}
		}
		
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(uniqueIdentifier(MOTOR_TIMESTAMP_PROPERTY, hiveId, motorIndex), DEFAULT_MOTOR_TIMESTAMP).equals(date)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(uniqueIdentifier(MOTOR_TIMESTAMP_PROPERTY, hiveId, motorIndex), date);
				editor.commit();
			}
		}
	}
}
