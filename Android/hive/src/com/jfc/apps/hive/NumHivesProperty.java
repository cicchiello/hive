package com.jfc.apps.hive;

import android.app.Activity;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;


public class NumHivesProperty {
	private static final String TAG = NumHivesProperty.class.getName();

	private static final String NUM_HIVES_PROPERTY = "NUM_HIVES";
	private static final String DEFAULT_NUM_HIVES = "0";
	
	public static boolean isNumHivesPropertyDefined(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (SP.contains(NUM_HIVES_PROPERTY) && 
			!SP.getString(NUM_HIVES_PROPERTY, DEFAULT_NUM_HIVES).equals(DEFAULT_NUM_HIVES)) {
			return true;
		} else {
			return false;
		}
	}
	
	public static int getNumHivesProperty(Activity activity) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		String v = SP.getString(NUM_HIVES_PROPERTY, DEFAULT_NUM_HIVES);
		return Integer.parseInt(v);
	}
	
	public static void clearNumHivesProperty(Activity activity) {
		resetNumHivesProperty(activity);
	}
	
	private static void resetNumHivesProperty(Activity activity) {
		setNumHivesProperty(activity, 0);
	}
	
	public static void setNumHivesProperty(Activity activity, int value) {
		String valueStr = Integer.toString(value);
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity.getBaseContext());
		if (!SP.getString(NUM_HIVES_PROPERTY, DEFAULT_NUM_HIVES).equals(valueStr)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(NUM_HIVES_PROPERTY, valueStr);
			editor.commit();
		}
	}
}
