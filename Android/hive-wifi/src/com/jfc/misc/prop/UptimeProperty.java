package com.jfc.misc.prop;

import java.util.Calendar;
import java.util.Locale;

import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import com.jfc.apps.hive.HiveEnv.CouchGetConfig_onCompletion;
import com.jfc.apps.hive.HiveEnv;
import com.jfc.apps.hive.R;
import com.jfc.apps.hive.SensorSampleRateProperty;
import com.jfc.srvc.cloud.CouchGetBackground;
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
import android.util.Log;
import android.view.View;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;


public class UptimeProperty implements IPropertyMgr {
	private static final String TAG = UptimeProperty.class.getName();

	private static final String UPTIME_PROPERTY = "UPTIME";
	private static final String UPTIME_TIMESTAMP = "UPTIME_TIMESTAMP";
	private static final String EMBEDDED_CONFIG_PROPERTY = "EMBEDDED_CONFIG";
	private static final String DEFAULT_UPTIME = "0";
	private static final String DEFAULT_UPTIME_TIMESTAMP = "0";
	private static final String DEFAULT_EMBEDDED_CONFIG = "{}";

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
					cal.setTimeInMillis(1000*UptimeProperty.getUptime(mActivity, hiveIndex));
					upsinceValueStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
		        }
		        
		        TextView upsinceTv = (TextView) mAlert.findViewById(R.id.upsince_text);
		        TextView statusTv = (TextView) mAlert.findViewById(R.id.current_status_text);
		        TextView embeddedVersionTv = (TextView) mAlert.findViewById(R.id.embedded_version_text);
		        upsinceTv.setText(upsinceValueStr);
		        
		        if (isUptimePropertyDefined(mActivity, hiveIndex)) {
		        	long uptimeTimestamp_s = getUptimeTimestamp(mActivity, hiveIndex);
		        	long uptimeTimestamp_ms = 1000*uptimeTimestamp_s;
		        	Calendar cal = Calendar.getInstance(Locale.ENGLISH);
		        	cal.setTimeInMillis(uptimeTimestamp_ms);
					String heartbeatTimeStr = DateFormat.format("dd-MMM-yy HH:mm",  cal).toString();
					int rate_minutes = SensorSampleRateProperty.getRate(mActivity);
					int second_ms = 1000;
					int minute_s = 60;
					long timeToBeSureOfDeath_ms = uptimeTimestamp_ms + (rate_minutes+1)*minute_s*second_ms;
					if (System.currentTimeMillis() > timeToBeSureOfDeath_ms) 
						statusTv.setText("Nothing since "+heartbeatTimeStr);
					else 
						statusTv.setText("Up as of "+heartbeatTimeStr);
		        } else statusTv.setText("Unknown");
		        
		        JSONObject config = UptimeProperty.getEmbeddedConfig(mActivity, hiveIndex);
		        try {
					embeddedVersionTv.setText(config.has("hive-version") ? config.getString("hive-version") : "e.f.g");
				} catch (JSONException e) {
					embeddedVersionTv.setText("e.f.g");
				}
			}
		};
		uptimeButton.setOnClickListener(ocl);
		
		HiveEnv.CouchGetConfig_onCompletion onCompletion = new HiveEnv.CouchGetConfig_onCompletion() {
			@Override
			public void failed(final String msg) {
				mActivity.runOnUiThread(new Runnable() {public void run() {Toast.makeText(mActivity, msg, Toast.LENGTH_LONG).show();}});
			}

			@Override
			public void complete(final JSONObject doc) {
				mActivity.runOnUiThread(new Runnable() {
					public void run() {setEmbeddedConfig(mActivity, ActiveHiveProperty.getActiveHiveIndex(mActivity), doc);}
				});
			}
		};
		
		HiveEnv.couchGetConfig(mActivity, onCompletion);
	}

	
	public static boolean isUptimePropertyDefined(Context ctxt, int hiveIndex) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		if (SP.contains(UPTIME_PROPERTY+Integer.toString(hiveIndex)) && 
			!SP.getString(UPTIME_PROPERTY+Integer.toString(hiveIndex), DEFAULT_UPTIME).equals(DEFAULT_UPTIME) &&
			SP.contains(UPTIME_TIMESTAMP+Integer.toString(hiveIndex)) &&
			!SP.getString(UPTIME_TIMESTAMP+Integer.toString(hiveIndex), DEFAULT_UPTIME_TIMESTAMP).equals(DEFAULT_UPTIME_TIMESTAMP)) {
			return true;
		} else {
			return false;
		}
	}
	
	public static long getUptime(Context ctxt, int hiveIndex) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		String v = SP.getString(UPTIME_PROPERTY+Integer.toString(hiveIndex), DEFAULT_UPTIME);
		return Long.parseLong(v);
	}
	
	public static long getUptimeTimestamp(Context ctxt, int hiveIndex) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		String v = SP.getString(UPTIME_TIMESTAMP+Integer.toString(hiveIndex), DEFAULT_UPTIME_TIMESTAMP);
		return Long.parseLong(v);
	}

	public static JSONObject getEmbeddedConfig(Context ctxt, int hiveIndex) {
		try {
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
			String v = SP.getString(EMBEDDED_CONFIG_PROPERTY+Integer.toString(hiveIndex), DEFAULT_EMBEDDED_CONFIG);
			return new JSONObject(new JSONTokener(v));
		} catch (JSONException e) {
			Log.e(TAG, "Error parsing embedded config JSON doc");
			return null;
		}
	}
	
	public static void clearUptimeProperty(Context ctxt, int hiveIndex) {
		resetUptimeProperty(ctxt.getApplicationContext(), hiveIndex);
	}
	
	private static void resetUptimeProperty(Context ctxt, int hiveIndex) {
		setUptimeProperty(ctxt.getApplicationContext(), hiveIndex, 0, 0);
	}
	
	public static void setUptimeProperty(Context ctxt, int hiveIndex, long bootTime, long timestamp) {
		String bootTimeStr = Long.toString(bootTime);
		String timestampStr = Long.toString(timestamp);
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(UPTIME_PROPERTY+Integer.toString(hiveIndex), DEFAULT_UPTIME).equals(bootTimeStr)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(UPTIME_PROPERTY+Integer.toString(hiveIndex), bootTimeStr);
			editor.commit();
		}
		if (!SP.getString(UPTIME_TIMESTAMP+Integer.toString(hiveIndex), DEFAULT_UPTIME_TIMESTAMP).equals(timestampStr)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(UPTIME_TIMESTAMP+Integer.toString(hiveIndex), timestampStr);
			editor.commit();
		}
	}

	public static void setEmbeddedConfig(Context ctxt, int hiveIndex, JSONObject configDoc) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(EMBEDDED_CONFIG_PROPERTY+Integer.toString(hiveIndex), DEFAULT_EMBEDDED_CONFIG).equals(configDoc.toString())) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(EMBEDDED_CONFIG_PROPERTY+Integer.toString(hiveIndex), configDoc.toString());
			editor.commit();
		}
	}

	static public void display(final Activity activity, final int uptimeResid, final int uptimeTimestampResid) {
		final int hiveIndex = ActiveHiveProperty.getActiveHiveIndex(activity);
		if (UptimeProperty.isUptimePropertyDefined(activity, hiveIndex)) {
			final long uptimeMillis = UptimeProperty.getUptime(activity, hiveIndex)*1000;
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
