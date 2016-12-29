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
import android.widget.ImageButton;
import android.widget.TextView;

import com.example.hive.R;
import com.jfc.util.misc.SplashyText;

public class MotorProperty {
	private static final String TAG = MotorProperty.class.getName();

	private static final String MOTOR_VALUE_PROPERTY = "MOTOR_VALUE_PROPERTY";
	private static final String MOTOR_DATE_PROPERTY = "MOTOR_DATE_PROPERTY";
	private static final String DEFAULT_MOTOR_VALUE = "0";
	private static final String DEFAULT_MOTOR_DATE = "<TBD>";
	
	private AlertDialog alert;
	private Activity activity;
	private TextView value, timestamp;
	private int index;
	
	public MotorProperty(final Activity activity, final int index, final TextView value, ImageButton button, final TextView timestamp) {
		this.activity = activity;
		this.value = value;
		this.timestamp = timestamp;
		this.index = index;
		
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
				builder.setTitle("Motor "+Integer.toString(index+1));
				builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
		        	@Override
		        	public void onClick(DialogInterface dialog, int which) {
		        		postToDb(((TextView)alert.findViewById(R.id.textValue)).getText().toString());
		        		alert.dismiss(); 
		        		alert = null;
		        	}
		        });
		        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {alert.dismiss(); alert = null;}
		        });
		        alert = builder.show();
		        
		        TextView tv = (TextView) alert.findViewById(R.id.textValue);
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
		TextView tv = (TextView) getAlertDialog().findViewById(R.id.textValue);
		int n = Integer.parseInt(tv.getText().toString())+1;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(activity, tv);
	}
	
	private void inc100() {
		TextView tv = (TextView) getAlertDialog().findViewById(R.id.textValue);
		int n = Integer.parseInt(tv.getText().toString())+100;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(activity, tv);
	}
	
	private void dec() {
		TextView tv = (TextView) getAlertDialog().findViewById(R.id.textValue);
		int n = Integer.parseInt(tv.getText().toString())-1;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(activity, tv);
	}
	
	private void dec100() {
		TextView tv = (TextView) getAlertDialog().findViewById(R.id.textValue);
		int n = Integer.parseInt(tv.getText().toString())-100;
		tv.setText(Integer.toString(n));
		SplashyText.highlightModifiedField(activity, tv);
	}
	
	private void postToDb(String valueStr) {
		String ActiveHive = ActiveHiveProperty.getActiveHiveProperty(activity);
		String HiveId = HiveEnv.getHiveAddress(activity, ActiveHive);
		boolean haveHiveId = HiveId != null;
		
		if (haveHiveId) {
			HiveId = HiveId.replace(':', '-');
			
			PostActuatorBackground.ResultCallback onCompletion = new PostActuatorBackground.ResultCallback() {
				@Override
		    	public void success(String id, String rev) {
				}
				
				@Override
				public void error(String msg) {
					Log.e(TAG, "Error: "+msg);
				}
			};

			String sensor = "motor"+Integer.toString(index)+"-target";
			String rangeStartKeyClause = "[\"" + HiveId + "\",\""+sensor+"\",\"99999999\"]";
			String rangeEndKeyClause = "[\"" +HiveId + "\",\""+sensor+"\",\"00000000\"]";
			String query = "_design/SensorLog/_view/by-hive-sensor?endKey=" + rangeEndKeyClause + "&startkey=" + rangeStartKeyClause + "&descending=true&limit=1";

			JSONObject doc = new JSONObject();
			try {
				doc.put("hiveid", HiveId);
				doc.put("sensor", sensor);
				doc.put("timestamp", Long.toString(System.currentTimeMillis()));
				doc.put("value", valueStr);
			} catch (JSONException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			
			new PostActuatorBackground(HiveEnv.DbHost, HiveEnv.DbPort, HiveEnv.Db, doc, onCompletion).execute();
		}
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
		if (SP.contains(MOTOR_VALUE_PROPERTY+Integer.toString(index)) && 
			!SP.getString(MOTOR_VALUE_PROPERTY+Integer.toString(index), DEFAULT_MOTOR_VALUE).equals(DEFAULT_MOTOR_VALUE)) {
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
		String v = SP.getString(MOTOR_VALUE_PROPERTY+Integer.toString(index), DEFAULT_MOTOR_VALUE);
		return v;
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
		setMotorProperty(activity, index, DEFAULT_MOTOR_VALUE, DEFAULT_MOTOR_DATE);
	}
	
	public static void setMotorProperty(Activity activity, int index, String value, long date) {
		setMotorProperty(activity, index, value, date==0 ? DEFAULT_MOTOR_DATE : Long.toString(date));
	}

	private static void setMotorProperty(Activity activity, int index, String value, String date) {
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(MOTOR_VALUE_PROPERTY+Integer.toString(index), DEFAULT_MOTOR_VALUE).equals(value)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(MOTOR_VALUE_PROPERTY+Integer.toString(index), value);
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
