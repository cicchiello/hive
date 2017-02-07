package com.jfc.apps.hive;

import android.app.Activity;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;


public class TempProperty {
	private static final String TAG = TempProperty.class.getName();

	private static final String TEMP_VALUE_PROPERTY = "TEMP_VALUE_PROPERTY";
	private static final String TEMP_DATE_PROPERTY = "TEMP_DATE_PROPERTY";
	private static final String DEFAULT_TEMP_VALUE = "<TBD>";
	private static final String DEFAULT_TEMP_DATE = "<TBD>";
	
	public static boolean isTempPropertyDefined(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (SP.contains(TEMP_VALUE_PROPERTY) && 
				!SP.getString(TEMP_VALUE_PROPERTY, DEFAULT_TEMP_VALUE).equals(DEFAULT_TEMP_VALUE)) {
				if (SP.contains(TEMP_DATE_PROPERTY) &&
					!SP.getString(TEMP_DATE_PROPERTY, DEFAULT_TEMP_DATE).equals(DEFAULT_TEMP_DATE)) 
					return true;
				else
					return false;
			} else
				return false;
	}
	
	public static String getTempValue(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(TEMP_VALUE_PROPERTY, DEFAULT_TEMP_VALUE);
		return v;
	}
	
	public static long getTempDate(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(TEMP_DATE_PROPERTY, DEFAULT_TEMP_DATE);
		try {
			return Long.parseLong(v);
		} catch (NumberFormatException nfe) {
			return 0;
		}
	}
	
	public static void resetTempProperty(Activity activity) {
		setTempProperty(activity, DEFAULT_TEMP_VALUE, DEFAULT_TEMP_DATE);
	}
	
	public static void setTempProperty(Activity activity, String value, long date) {
		setTempProperty(activity, value, date==0 ? DEFAULT_TEMP_DATE : Long.toString(date));
	}

	private static void setTempProperty(Activity activity, String value, String date) {
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(TEMP_VALUE_PROPERTY, DEFAULT_TEMP_VALUE).equals(value)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(TEMP_VALUE_PROPERTY, value);
				editor.commit();
			}
		}
		
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(TEMP_DATE_PROPERTY, DEFAULT_TEMP_DATE).equals(date)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(TEMP_DATE_PROPERTY, date);
				editor.commit();
			}
		}
	}
}
