package com.jfc.misc.prop;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;


public class UptimeProperty {
	private static final String TAG = UptimeProperty.class.getName();

	private static final String UPTIME_PROPERTY = "UPTIME";
	private static final String DEFAULT_UPTIME = "0";
	
	public static boolean isUptimePropertyDefined(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		if (SP.contains(UPTIME_PROPERTY) && 
			!SP.getString(UPTIME_PROPERTY, DEFAULT_UPTIME).equals(DEFAULT_UPTIME)) {
			return true;
		} else {
			return false;
		}
	}
	
	public static long getUptimeProperty(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		String v = SP.getString(UPTIME_PROPERTY, DEFAULT_UPTIME);
		return Long.parseLong(v);
	}
	
	public static void clearUptimeProperty(Context ctxt) {
		resetUptimeProperty(ctxt.getApplicationContext());
	}
	
	private static void resetUptimeProperty(Context ctxt) {
		setUptimeProperty(ctxt.getApplicationContext(), 0);
	}
	
	public static void setUptimeProperty(Context ctxt, long value) {
		String valueStr = Long.toString(value);
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(UPTIME_PROPERTY, DEFAULT_UPTIME).equals(valueStr)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(UPTIME_PROPERTY, valueStr);
			editor.commit();
		}
	}

}
