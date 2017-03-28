package com.jfc.misc.prop;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.widget.ImageButton;
import android.widget.TextView;

import com.jfc.apps.hive.HiveEnv;


public class IntProperty implements IPropertyMgr {
	private static final String TAG = IntProperty.class.getName();

    // created on constructions -- no need to save on pause
	private String mPropId;
	private int mDefVal;
	
	protected TextView mTV;
	protected Activity mActivity;

	// transient variables -- no need to save on pause
	protected AlertDialog mAlert;

	public IntProperty(final Activity activity, String propId, int defVal, final TextView tv, ImageButton button) {
		this.mPropId = propId;
		this.mDefVal = defVal;
		this.mActivity = activity;
		this.mTV = tv;
		
		if (isDefined(activity, mPropId, defVal)) {
			display(getVal(mActivity, mPropId, mDefVal));
		} else {
			setUndefined(mDefVal);
    	}
		mTV.setBackgroundColor(HiveEnv.ModifiableFieldBackgroundColor);
	}

	public AlertDialog getAlertDialog() {return mAlert;}

	protected static boolean isDefined(Context ctxt, String propId, int defVal) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		return SP.contains(propId) && !SP.getString(propId, Integer.toString(defVal)).equals(Integer.toString(defVal));
	}

	protected static int getVal(Context ctxt, String propId, int defVal) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String valueStr = SP.getString(propId+"|val", Integer.toString(defVal));
		return Integer.parseInt(valueStr);
	}
	
	public static long getTimestamp(Context ctxt, String propId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String valueStr = SP.getString(propId+"|timestamp", "0");
		return Long.parseLong(valueStr);
	}
	
	protected static void set(Context ctxt, String propId, int val, long timestamp, int defVal) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(propId+"|val", Integer.toString(defVal)).equals(Integer.toString(val)) ||
			!SP.getString(propId+"|timestamp", "0").equals(Long.toString(timestamp))) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(propId+"|val", Integer.toString(val));
			editor.putString(propId+"|timestamp", Long.toString(timestamp));
			editor.commit();
		}
	}

	protected static void reset(Context ctxt, String propId, int defVal) {
		set(ctxt, propId, defVal, System.currentTimeMillis()/1000, defVal);
	}
	
	private void setUndefined(int defVal) {
		mTV.setText(Integer.toString(defVal));
	}
	
	protected void display(int val) {
		mTV.setText(Integer.toString(val));
	}
	
	protected void set(String propId, int val, long timestamp, int defVal) {
		set(mActivity, propId, val, timestamp, defVal);
		display(val);
	}
	
	@Override
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent) {
		return false;
	}

	@Override
	public void onPermissionResult(int requestCode, String[] permissions, int[] grantResults) {
		// intentionally unimplemented
	}

}
