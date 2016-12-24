package com.jfc.apps.hive;

import android.app.Activity;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;


public class HumidProperty {
	private static final String TAG = HumidProperty.class.getName();

	private static final String HUMID_VALUE_PROPERTY = "HUMID_VALUE_PROPERTY";
	private static final String HUMID_DATE_PROPERTY = "HUMID_DATE_PROPERTY";
	private static final String DEFAULT_HUMID_VALUE = "<TBD>";
	private static final String DEFAULT_HUMID_DATE = "<TBD>";
	
	public static boolean isHumidPropertyDefined(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (SP.contains(HUMID_VALUE_PROPERTY) && 
				!SP.getString(HUMID_VALUE_PROPERTY, DEFAULT_HUMID_VALUE).equals(DEFAULT_HUMID_VALUE)) {
				if (SP.contains(HUMID_DATE_PROPERTY) &&
					!SP.getString(HUMID_DATE_PROPERTY, DEFAULT_HUMID_DATE).equals(DEFAULT_HUMID_DATE)) 
					return true;
				else
					return false;
			} else
				return false;
	}
	
	public static String getHumidValue(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(HUMID_VALUE_PROPERTY, DEFAULT_HUMID_VALUE);
		return v;
	}
	
	public static long getHumidDate(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(HUMID_DATE_PROPERTY, DEFAULT_HUMID_DATE);
		return Long.parseLong(v);
	}
	
	public static void resetHumidProperty(Activity activity) {
		setHumidProperty(activity, DEFAULT_HUMID_VALUE, DEFAULT_HUMID_DATE);
	}
	
	public static void setHumidProperty(Activity activity, String value, long date) {
		setHumidProperty(activity, value, date==0 ? DEFAULT_HUMID_DATE : Long.toString(date));
	}

	private static void setHumidProperty(Activity activity, String value, String date) {
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(HUMID_VALUE_PROPERTY, DEFAULT_HUMID_VALUE).equals(value)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(HUMID_VALUE_PROPERTY, value);
				editor.commit();
			}
		}
		
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(HUMID_DATE_PROPERTY, DEFAULT_HUMID_DATE).equals(date)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(HUMID_DATE_PROPERTY, date);
				editor.commit();
			}
		}
	}
}
