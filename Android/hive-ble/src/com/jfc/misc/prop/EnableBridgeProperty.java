package com.jfc.misc.prop;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageView;

import com.jfc.apps.hive.HiveEnv;
import com.jfc.apps.hive.R;
import com.jfc.srvc.ble2cld.BluetoothPipeSrvc;


public class EnableBridgeProperty implements IPropertyMgr {
	@SuppressWarnings("unused")
	private static final String TAG = EnableBridgeProperty.class.getName();

    static final int grayColor = HiveEnv.ModifiableFieldBackgroundColor;

	private ImageView mEnableButton;
	private AlertDialog mAlert;
	private Activity mActivity;

	public EnableBridgeProperty(final Activity activity, final ImageView enableButton) {
		mActivity = activity;
		mEnableButton = enableButton;
		
		displayEnableBridge(mEnableButton, getEnableBridgeProperty(activity));

    	mEnableButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				boolean newValue = !getEnableBridgeProperty(mActivity);
				setEnableBridgeProperty(mActivity, newValue);
				displayEnableBridge(mEnableButton, newValue);
				
    			Intent ble2cldIntent= new Intent(mActivity, BluetoothPipeSrvc.class);
    			ble2cldIntent.putExtra("cmd", "setup");
    			mActivity.startService(ble2cldIntent);
			}
		});
	}
	
	private static final String ENABLE_BRIDGE_PROPERTY = "ENABLE_BRIDGE";
	private static final String DEFAULT_ENABLE_BRIDGE= "0";

	public static boolean getEnableBridgeProperty(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String v = SP.getString(ENABLE_BRIDGE_PROPERTY, DEFAULT_ENABLE_BRIDGE);
		return Integer.parseInt(v) == 0 ? false : true;
	}
	
	public static void setEnableBridgeProperty(Context ctxt, boolean value) {
		String valueStr = value ? "1" : "0";
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(ENABLE_BRIDGE_PROPERTY, DEFAULT_ENABLE_BRIDGE).equals(valueStr)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(ENABLE_BRIDGE_PROPERTY, valueStr);
			editor.commit();
		}
	}

	public AlertDialog getAlertDialog() {return mAlert;}
	public boolean onActivityResult(Activity activity, int requestCode, int resultCode, Intent intent) {return false;}

	private void displayEnableBridge(ImageView enableText, boolean v) {
		int drawable = v ? R.drawable.toggle_on : R.drawable.toggle_off;
		mEnableButton.setImageDrawable(mActivity.getDrawable(drawable));
	}

	@Override
	public boolean onActivityResult(int requestCode, int resultCode,
			Intent intent) {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void onPermissionResult(int requestCode, String[] permissions, int[] grantResults) {
	}

}
