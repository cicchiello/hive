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
	
	public static boolean isMCUTempPropertyDefined(Activity activity, String hiveId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (SP.contains(uniqueIdentifier(MCU_TEMP_VALUE_PROPERTY, hiveId)) && 
			!SP.getString(uniqueIdentifier(MCU_TEMP_VALUE_PROPERTY, hiveId), DEFAULT_MCU_TEMP_VALUE).equals(DEFAULT_MCU_TEMP_VALUE)) {
			if (SP.contains(uniqueIdentifier(MCU_TEMP_DATE_PROPERTY, hiveId)) &&
				!SP.getString(uniqueIdentifier(MCU_TEMP_DATE_PROPERTY, hiveId), DEFAULT_MCU_TEMP_DATE).equals(DEFAULT_MCU_TEMP_DATE)) 
				return true;
			else
				return false;
		} else
			return false;
	}
	
	static private String uniqueIdentifier(String base, String hiveId) {
		return base+"|"+hiveId;
	}
	
	
	public static String getMCUTempValue(Activity activity, String hiveId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(uniqueIdentifier(MCU_TEMP_VALUE_PROPERTY, hiveId), DEFAULT_MCU_TEMP_VALUE);
		return v;
	}
	
	public static long getMCUTempDate(Activity activity, String hiveId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(uniqueIdentifier(MCU_TEMP_DATE_PROPERTY, hiveId), DEFAULT_MCU_TEMP_DATE);
		try {
			return Long.parseLong(v);
		} catch (NumberFormatException nfe) {
			return 0;
		}
	}
	
	public static void resetMCUTempProperty(Activity activity, String hiveId) {
		setMCUTempProperty(activity, hiveId, DEFAULT_MCU_TEMP_VALUE, DEFAULT_MCU_TEMP_DATE);
	}
	
	public static void setMCUTempProperty(Activity activity, String hiveId, String value, long date) {
		setMCUTempProperty(activity, hiveId, value, date==0 ? DEFAULT_MCU_TEMP_DATE : Long.toString(date));
	}

	private static void setMCUTempProperty(Activity activity, String hiveId, String value, String date) {
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(uniqueIdentifier(MCU_TEMP_VALUE_PROPERTY, hiveId), DEFAULT_MCU_TEMP_VALUE).equals(value)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(MCU_TEMP_VALUE_PROPERTY, value);
				editor.commit();
			}
		}
		
		{
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
			if (!SP.getString(uniqueIdentifier(MCU_TEMP_DATE_PROPERTY, hiveId), DEFAULT_MCU_TEMP_DATE).equals(date)) {
				SharedPreferences.Editor editor = SP.edit();
				editor.putString(MCU_TEMP_DATE_PROPERTY, date);
				editor.commit();
			}
		}
	}
}
