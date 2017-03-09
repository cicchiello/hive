package com.jfc.misc.prop;

import java.util.Calendar;
import java.util.Locale;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.text.format.DateFormat;
import android.widget.TextView;

import com.jfc.apps.hive.HiveEnv;
import com.jfc.util.misc.SplashyText;


public class LatchProperty implements IPropertyMgr {
	@SuppressWarnings("unused")
	private static final String TAG = LatchProperty.class.getName();

    static final int grayColor = HiveEnv.ModifiableFieldBackgroundColor;

	private TextView mEnableText, mTimestampText;
	private AlertDialog mAlert;
	private Activity mActivity;

	public LatchProperty(final Activity activity, String HiveId, final TextView enableText, TextView _timestamp) {
		mActivity = activity;
		mEnableText = enableText;
		mTimestampText = _timestamp;
		mEnableText.setBackgroundColor(0xffff0000); // RED
		
		displayLatch(getLatchProperty(activity, HiveId), getLatchDate(activity, HiveId));
	}
	
	private static final String LATCH_PROPERTY = "LATCH_PROPERTY";
	private static final String LATCH_DATE = "LATCH_DATE";
	private static final String DEFAULT_LATCH_PROPERTY= "0";
	private static final String DEFAULT_LATCH_DATE = "<TBD>";

	static private String uniqueIdentifier(String base, String hiveId) {
		return base+"|"+hiveId;
	}
	
	public static boolean getLatchProperty(Context ctxt, String hiveId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String v = SP.getString(uniqueIdentifier(LATCH_PROPERTY, hiveId), DEFAULT_LATCH_PROPERTY);
		return Integer.parseInt(v) == 0 ? false : true;
	}
	
	public static long getLatchDate(Activity activity, String hiveId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(uniqueIdentifier(LATCH_DATE, hiveId), DEFAULT_LATCH_DATE);
		try {
			return Long.parseLong(v);
		} catch (NumberFormatException nfe) {
			return 0;
		}
	}
	
	public static void setLatchProperty(Context ctxt, String HiveId, String valueStr, long date) {
		valueStr = valueStr.equalsIgnoreCase("Open") ? "1" : "0";
		String dateStr = Long.toString(date);
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
			if (!SP.getString(uniqueIdentifier(LATCH_PROPERTY,HiveId), DEFAULT_LATCH_PROPERTY).equals(valueStr)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(DEFAULT_LATCH_PROPERTY, valueStr);
				editor.commit();
			}
		}
		
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
			if (!SP.getString(uniqueIdentifier(LATCH_DATE, HiveId), DEFAULT_LATCH_DATE).equals(dateStr)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(DEFAULT_LATCH_DATE, dateStr);
				editor.commit();
			}
		}
	}

	public AlertDialog getAlertDialog() {return mAlert;}
	public boolean onActivityResult(Activity activity, int requestCode, int resultCode, Intent intent) {return false;}

	private void displayLatch(boolean v, long t) {
		mEnableText.setText(v ? "Open" : "Closed");
		mEnableText.setBackgroundColor(grayColor);
		
		boolean splashValue = !mEnableText.getText().equals(v);
		mEnableText.setText(v ? "Open" : "Closed");
    	if (splashValue) 
    		SplashyText.highlightModifiedField(mActivity, mEnableText);
		Calendar cal = Calendar.getInstance(Locale.ENGLISH);
		cal.setTimeInMillis(t);
		String timestampStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
		boolean splashTimestamp = !mTimestampText.getText().equals(timestampStr);
		mTimestampText.setText(timestampStr);
		if (splashTimestamp) 
			SplashyText.highlightModifiedField(mActivity, mTimestampText);
	}

	@Override
	public boolean onActivityResult(int requestCode, int resultCode,
			Intent intent) {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void onPermissionResult(int requestCode, String[] permissions, int[] grantResults) {
	}

}
