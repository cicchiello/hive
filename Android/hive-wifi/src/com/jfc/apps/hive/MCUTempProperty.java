package com.jfc.apps.hive;

import android.app.Activity;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;


public class MCUTempProperty {
	private static final String TAG = MCUTempProperty.class.getName();

	private static final String MCU_TEMP_VALUE_PROPERTY = "MCU_TEMP_VALUE_PROPERTY";
	private static final String MCU_TEMP_DATE_PROPERTY = "MCU_TEMP_DATE_PROPERTY";
	private static final String DEFAULT_MCU_TEMP_VALUE = "<TBD>";
	private static final String DEFAULT_MCU_TEMP_DATE = "<TBD>";
	
	public static boolean isMCUTempPropertyDefined(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (SP.contains(MCU_TEMP_VALUE_PROPERTY) && 
				!SP.getString(MCU_TEMP_VALUE_PROPERTY, DEFAULT_MCU_TEMP_VALUE).equals(DEFAULT_MCU_TEMP_VALUE)) {
				if (SP.contains(MCU_TEMP_DATE_PROPERTY) &&
					!SP.getString(MCU_TEMP_DATE_PROPERTY, DEFAULT_MCU_TEMP_DATE).equals(DEFAULT_MCU_TEMP_DATE)) 
					return true;
				else
					return false;
			} else
				return false;
	}
	
	public static String getMCUTempValue(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(MCU_TEMP_VALUE_PROPERTY, DEFAULT_MCU_TEMP_VALUE);
		return v;
	}
	
	public static long getMCUTempDate(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(MCU_TEMP_DATE_PROPERTY, DEFAULT_MCU_TEMP_DATE);
		try {
			return Long.parseLong(v);
		} catch (NumberFormatException nfe) {
			return 0;
		}
	}
	
	public static void resetMCUTempProperty(Activity activity) {
		setMCUTempProperty(activity, DEFAULT_MCU_TEMP_VALUE, DEFAULT_MCU_TEMP_DATE);
	}
	
	public static void setMCUTempProperty(Activity activity, String value, long date) {
		setMCUTempProperty(activity, value, date==0 ? DEFAULT_MCU_TEMP_DATE : Long.toString(date));
	}

	private static void setMCUTempProperty(Activity activity, String value, String date) {
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(MCU_TEMP_VALUE_PROPERTY, DEFAULT_MCU_TEMP_VALUE).equals(value)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(MCU_TEMP_VALUE_PROPERTY, value);
				editor.commit();
			}
		}
		
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(MCU_TEMP_DATE_PROPERTY, DEFAULT_MCU_TEMP_DATE).equals(date)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(MCU_TEMP_DATE_PROPERTY, date);
				editor.commit();
			}
		}
	}
}
