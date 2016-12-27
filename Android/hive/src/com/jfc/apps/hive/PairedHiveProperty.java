package com.jfc.apps.hive;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;


public class PairedHiveProperty {
	private static final String TAG = PairedHiveProperty.class.getName();

	public static final String DEFAULT_PAIRED_HIVE_NAME = "MyHive";
	
	private static final String PAIRED_HIVES_PROPERTY = "PAIRED_HIVES";
	private static final String DEFAULT_PAIRED_HIVE_ID = "<undefined>";
	
	private static String getIdIdentifier(int index) {
		return PAIRED_HIVES_PROPERTY+"["+index+"].id";
	}
	
	private static String getNameIdentifier(int index) {
		return PAIRED_HIVES_PROPERTY+"["+index+"].name";
	}
	
	public static String getPairedHiveId(Context ctxt, int index) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String v = SP.getString(getIdIdentifier(index), DEFAULT_PAIRED_HIVE_ID);
		return v;
	}
	
	public static String getPairedHiveName(Context ctxt, int index) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String v = SP.getString(getNameIdentifier(index), DEFAULT_PAIRED_HIVE_NAME);
		return v;
	}
	
	public static void setPairedHiveId(Context ctxt, int index, String value) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		if (!SP.getString(getIdIdentifier(index), DEFAULT_PAIRED_HIVE_ID).equals(value)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(getIdIdentifier(index), value);
			editor.commit();
		}
	}
	
	public static void setPairedHiveName(Context ctxt, int index, String value) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		if (!SP.getString(getNameIdentifier(index), DEFAULT_PAIRED_HIVE_NAME).equals(value)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(getNameIdentifier(index), value);
			editor.commit();
		}
	}
}
