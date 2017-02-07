package com.jfc.misc.prop;

import java.util.Calendar;
import java.util.Locale;

import com.jfc.apps.hive.R;
import com.jfc.srvc.ble2cld.BluetoothPipeSrvc;
import com.jfc.util.misc.SplashyText;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.text.format.DateFormat;
import android.text.format.DateUtils;
import android.view.View;
import android.widget.ImageButton;
import android.widget.TextView;


public class UptimeProperty implements IPropertyMgr {
	private static final String TAG = UptimeProperty.class.getName();

	private static final String UPTIME_PROPERTY = "UPTIME";
	private static final String EMBEDDED_VERSION_PROPERTY = "EMBEDDED_VERSION";
	private static final String DEFAULT_UPTIME = "0";
	private static final String DEFAULT_EMBEDDED_VERSION = "0.0.0";

	private Activity mActivity;
	private AlertDialog mAlert;

	public UptimeProperty(Activity _activity, TextView _value, ImageButton uptimeButton, TextView _timestamp) {
		mActivity = _activity;
		
		View.OnClickListener ocl = new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				int hiveIndex = ActiveHiveProperty.getActiveHiveIndex(mActivity);
				AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
				
				builder.setIcon(R.drawable.ic_hive);
				builder.setView(R.layout.uptime_dialog);
				builder.setTitle(R.string.uptime_dialog_title);
		        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
		        });
		        mAlert = builder.show();

		        String upsinceValueStr = "unknown";
		        if (UptimeProperty.isUptimePropertyDefined(mActivity, hiveIndex)) {
					Calendar cal = Calendar.getInstance(Locale.ENGLISH);
					cal.setTimeInMillis(1000*UptimeProperty.getUptimeProperty(mActivity, hiveIndex));
					upsinceValueStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
		        }
		        
		        TextView upsinceTv = (TextView) mAlert.findViewById(R.id.upsince_text);
		        TextView statusTv = (TextView) mAlert.findViewById(R.id.current_status_text);
		        TextView embeddedVersionTv = (TextView) mAlert.findViewById(R.id.embedded_version_text);
		        upsinceTv.setText(upsinceValueStr);
		        boolean isConnected = BluetoothPipeSrvc.isConnected(mActivity);
		        statusTv.setText(isConnected ? "Connected" : "Unknown");
		        embeddedVersionTv.setText(UptimeProperty.getEmbeddedVersion(mActivity, hiveIndex));
			}
		};
		uptimeButton.setOnClickListener(ocl);
	}
	
	public static boolean isUptimePropertyDefined(Context ctxt, int hiveIndex) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		if (SP.contains(UPTIME_PROPERTY+Integer.toString(hiveIndex)) && 
			!SP.getString(UPTIME_PROPERTY+Integer.toString(hiveIndex), DEFAULT_UPTIME).equals(DEFAULT_UPTIME)) {
			return true;
		} else {
			return false;
		}
	}
	
	public static long getUptimeProperty(Context ctxt, int hiveIndex) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		String v = SP.getString(UPTIME_PROPERTY+Integer.toString(hiveIndex), DEFAULT_UPTIME);
		return Long.parseLong(v);
	}
	
	public static String getEmbeddedVersion(Context ctxt, int hiveIndex) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		String v = SP.getString(EMBEDDED_VERSION_PROPERTY+Integer.toString(hiveIndex), DEFAULT_EMBEDDED_VERSION);
		return v;
	}
	
	public static void clearUptimeProperty(Context ctxt, int hiveIndex) {
		resetUptimeProperty(ctxt.getApplicationContext(), hiveIndex);
	}
	
	private static void resetUptimeProperty(Context ctxt, int hiveIndex) {
		setUptimeProperty(ctxt.getApplicationContext(), hiveIndex, 0);
	}
	
	public static void setUptimeProperty(Context ctxt, int hiveIndex, long value) {
		String valueStr = Long.toString(value);
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(UPTIME_PROPERTY+Integer.toString(hiveIndex), DEFAULT_UPTIME).equals(valueStr)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(UPTIME_PROPERTY+Integer.toString(hiveIndex), valueStr);
			editor.commit();
		}
	}

	public static void setEmbeddedVersion(Context ctxt, int hiveIndex, String version) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(EMBEDDED_VERSION_PROPERTY+Integer.toString(hiveIndex), DEFAULT_EMBEDDED_VERSION).equals(version)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(EMBEDDED_VERSION_PROPERTY+Integer.toString(hiveIndex), version);
			editor.commit();
		}
	}

	static public void display(final Activity activity, final int uptimeResid, final int uptimeTimestampResid) {
		final int hiveIndex = ActiveHiveProperty.getActiveHiveIndex(activity);
		if (UptimeProperty.isUptimePropertyDefined(activity, hiveIndex)) {
			final long uptimeMillis = UptimeProperty.getUptimeProperty(activity, hiveIndex)*1000;
			final CharSequence since = DateUtils.getRelativeTimeSpanString(uptimeMillis,
					System.currentTimeMillis(), DateUtils.MINUTE_IN_MILLIS, DateUtils.FORMAT_ABBREV_RELATIVE);
			activity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
		    		TextView uptimeTv = (TextView) activity.findViewById(uptimeResid);
		    		if (!since.equals(uptimeTv.getText().toString())) {
		    			uptimeTv.setText(since);
						SplashyText.highlightModifiedField(activity, uptimeTv);
		    		}
		    		TextView uptimeTimestampTv = (TextView) activity.findViewById(uptimeTimestampResid);
					Calendar cal = Calendar.getInstance(Locale.ENGLISH);
					cal.setTimeInMillis(System.currentTimeMillis());
					final String timestampStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
					if (!timestampStr.equals(uptimeTimestampTv.getText().toString())) {
						uptimeTimestampTv.setText(timestampStr);
						SplashyText.highlightModifiedField(activity, uptimeTimestampTv);
					}
				}
			});
		}
	}
	
	public AlertDialog getAlertDialog() {return mAlert;}
	public boolean onActivityResult(Activity activity, int requestCode, int resultCode, Intent intent) {return false;}

	@Override
	public boolean onActivityResult(int requestCode, int resultCode,
			Intent intent) {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void onPermissionResult(int requestCode, String[] permissions,
			int[] grantResults) {
		// TODO Auto-generated method stub
		
	}

}
