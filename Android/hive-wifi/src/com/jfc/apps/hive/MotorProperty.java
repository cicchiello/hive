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

	private static final String MOTOR_STEPS_PROPERTY = "MOTOR_VALUE_PROPERTY";
	private static final String MOTOR_DATE_PROPERTY = "MOTOR_DATE_PROPERTY";
	private static final String DEFAULT_MOTOR_STEPS = "0";
	private static final String DEFAULT_MOTOR_DATE = "<TBD>";
	
	private AlertDialog alert;
	private Activity activity;
	private TextView value, timestamp;
	private int index;
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
	
	public MotorProperty(Activity _activity, int _index, TextView _value, ImageButton button, TextView _timestamp) {
		this.activity = _activity;
		this.value = _value;
		this.timestamp = _timestamp;
		this.index = _index;
		this.hiveId = HiveEnv.getHiveAddress(activity, ActiveHiveProperty.getActiveHiveName(activity));

		if (isMotorPropertyDefined(activity, index)) {
			setUndefined();
		} else {
			display(getMotorValue(activity, index), getMotorDate(activity, index));
		}
		
    	button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				AlertDialog.Builder builder = new AlertDialog.Builder(activity);
				
				builder.setIcon(R.drawable.ic_hive);
				builder.setView(R.layout.motor_dialog);
				builder.setTitle("Motor "+Integer.toString(index+1)+" distance (mm)");
				builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
		        	@Override
		        	public void onClick(DialogInterface dialog, int which) {
						EditText et = (EditText) alert.findViewById(R.id.textValue);
						String linearDistanceMillimetersStr = et.getText().toString();
						double linearDistanceMillimeters = Long.parseLong(linearDistanceMillimetersStr);
						long steps = linearDistanceToSteps(activity, linearDistanceMillimeters/1000.0);
						
		        		postToDb(Long.toString(steps));

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
		        tv.setText(value.getText());

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
			final String sensor = "motor"+Integer.toString(index)+"-target";
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
		value.setText("????");
		timestamp.setText("????");
	}
	
	private void display(String v, long t) {
		boolean splashValue = !value.getText().equals(v);
    	value.setText(v);
    	if (splashValue) 
    		SplashyText.highlightModifiedField(activity, value);
		Calendar cal = Calendar.getInstance(Locale.ENGLISH);
		cal.setTimeInMillis(t);
		String timestampStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
		boolean splashTimestamp = !timestamp.getText().equals(timestampStr);
		timestamp.setText(timestampStr);
		if (splashTimestamp) 
			SplashyText.highlightModifiedField(activity, timestamp);
	}

	
	public static boolean isMotorPropertyDefined(Activity activity, int index) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (SP.contains(MOTOR_STEPS_PROPERTY+Integer.toString(index)) && 
			!SP.getString(MOTOR_STEPS_PROPERTY+Integer.toString(index), DEFAULT_MOTOR_STEPS).equals(DEFAULT_MOTOR_STEPS)) {
			if (SP.contains(MOTOR_DATE_PROPERTY+Integer.toString(index)) &&
				!SP.getString(MOTOR_DATE_PROPERTY+Integer.toString(index), DEFAULT_MOTOR_DATE).equals(DEFAULT_MOTOR_DATE)) 	
				return true;
			else
				return false;
		} else
			return false;
	}
	
	
	public static String getMotorValue(Activity activity, int index) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String stepsStr = SP.getString(MOTOR_STEPS_PROPERTY+Integer.toString(index), DEFAULT_MOTOR_STEPS);
		double meters = stepsToLinearDistance(activity, Long.parseLong(stepsStr));
		double millimeters = meters*1000.0;
		return Long.toString((long) (millimeters+0.5));
	}
	
	public static long getMotorDate(Activity activity, int index) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(MOTOR_DATE_PROPERTY+Integer.toString(index), DEFAULT_MOTOR_DATE);
		try {
			return Long.parseLong(v);
		} catch (NumberFormatException nfe) {
			return 0;
		}
	}
	
	public static void resetMotorProperty(Activity activity, int index) {
		setMotorProperty(activity, index, DEFAULT_MOTOR_STEPS, DEFAULT_MOTOR_DATE);
	}
	
	public static void setMotorProperty(Activity activity, int index, String steps, long date) {
		setMotorProperty(activity, index, steps, date==0 ? DEFAULT_MOTOR_DATE : Long.toString(date));
	}

	private static void setMotorProperty(Activity activity, int index, String steps, String date) {
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(MOTOR_STEPS_PROPERTY+Integer.toString(index), DEFAULT_MOTOR_STEPS).equals(steps)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(MOTOR_STEPS_PROPERTY+Integer.toString(index), steps);
				editor.commit();
			}
		}
		
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(MOTOR_DATE_PROPERTY+Integer.toString(index), DEFAULT_MOTOR_DATE).equals(date)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(MOTOR_DATE_PROPERTY+Integer.toString(index), date);
				editor.commit();
			}
		}
	}
}
