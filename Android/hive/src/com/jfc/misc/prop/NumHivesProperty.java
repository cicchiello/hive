package com.jfc.misc.prop;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;


public class NumHivesProperty {
	private static final String TAG = NumHivesProperty.class.getName();

	private static final String NUM_HIVES_PROPERTY = "NUM_HIVES";
	private static final String DEFAULT_NUM_HIVES = "0";
	
	public static boolean isNumHivesPropertyDefined(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		if (SP.contains(NUM_HIVES_PROPERTY) && 
			!SP.getString(NUM_HIVES_PROPERTY, DEFAULT_NUM_HIVES).equals(DEFAULT_NUM_HIVES)) {
			return true;
		} else {
			return false;
		}
	}
	
	public static int getNumHivesProperty(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		String v = SP.getString(NUM_HIVES_PROPERTY, DEFAULT_NUM_HIVES);
		return Integer.parseInt(v);
	}
	
	public static void clearNumHivesProperty(Context ctxt) {
		resetNumHivesProperty(ctxt.getApplicationContext());
	}
	
	private static void resetNumHivesProperty(Context ctxt) {
		setNumHivesProperty(ctxt.getApplicationContext(), 0);
	}
	
	public static void setNumHivesProperty(Context ctxt, int value) {
		String valueStr = Integer.toString(value);
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(NUM_HIVES_PROPERTY, DEFAULT_NUM_HIVES).equals(valueStr)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(NUM_HIVES_PROPERTY, valueStr);
			editor.commit();
		}
	}
}
