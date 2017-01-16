package com.jfc.apps.hive;

import android.app.Activity;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;


public class BeeCntProperty {
	private static final String TAG = BeeCntProperty.class.getName();

	private static final String BEECNT_VALUE_PROPERTY = "BEECNT_VALUE_PROPERTY";
	private static final String BEECNT_DATE_PROPERTY = "BEECNT_DATE_PROPERTY";
	private static final String DEFAULT_BEECNT_VALUE = "0";
	private static final String DEFAULT_BEECNT_DATE = "<TBD>";
	
	public static boolean isBeeCntPropertyDefined(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (SP.contains(BEECNT_VALUE_PROPERTY) && 
				!SP.getString(BEECNT_VALUE_PROPERTY, DEFAULT_BEECNT_VALUE).equals(DEFAULT_BEECNT_VALUE)) {
				if (SP.contains(BEECNT_DATE_PROPERTY) &&
					!SP.getString(BEECNT_DATE_PROPERTY, DEFAULT_BEECNT_DATE).equals(DEFAULT_BEECNT_DATE)) 
					return true;
				else
					return false;
			} else
				return false;
	}
	
	public static String getBeeCntValue(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(BEECNT_VALUE_PROPERTY, DEFAULT_BEECNT_VALUE);
		return v;
	}
	
	public static long getBeeCntDate(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(BEECNT_DATE_PROPERTY, DEFAULT_BEECNT_DATE);
		try {
			return Long.parseLong(v);
		} catch (NumberFormatException nfe) {
			return 0;
		}
	}
	
	public static void resetBeeCntProperty(Activity activity) {
		setBeeCntProperty(activity, DEFAULT_BEECNT_VALUE, DEFAULT_BEECNT_DATE);
	}
	
	public static void setBeeCntProperty(Activity activity, String value, long date) {
		setBeeCntProperty(activity, value, date==0 ? DEFAULT_BEECNT_DATE : Long.toString(date));
	}

	private static void setBeeCntProperty(Activity activity, String value, String date) {
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(BEECNT_VALUE_PROPERTY, DEFAULT_BEECNT_VALUE).equals(value)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(BEECNT_VALUE_PROPERTY, value);
				editor.commit();
			}
		}
		
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(BEECNT_DATE_PROPERTY, DEFAULT_BEECNT_DATE).equals(date)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(BEECNT_DATE_PROPERTY, date);
				editor.commit();
			}
		}
	}
}
