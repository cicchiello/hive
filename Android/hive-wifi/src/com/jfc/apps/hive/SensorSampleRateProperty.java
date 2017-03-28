package com.jfc.apps.hive;

import org.acra.ACRA;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;

import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.IPropertyMgr;
import com.jfc.misc.prop.UptimeProperty;
import com.jfc.srvc.cloud.CouchCmdPush;
import com.jfc.util.misc.DbAlertHandler;
import com.jfc.util.misc.DialogUtils;
import com.jfc.util.misc.SplashyText;


public class SensorSampleRateProperty implements IPropertyMgr {
	private static final String TAG = SensorSampleRateProperty.class.getName();

	private static final String SAMPLE_RATE = "SAMPLE_RATE";
	private static final String SAMPLE_RATE_TIMESTAMP = "SAMPLE_RATE_TIMESTAMP";
	private static final int DEFAULT_SAMPLE_RATE = 5;
	private static final String DEFAULT_SAMPLE_RATE_TIMESTAMP = "0";
	
	private static final String EMBEDDED_SAMPLE_RATE_PROPERTY = "sensor-rate-seconds";
	
    // created on constructions -- no need to save on pause
	private TextView mSampleRateTv;
	private Activity mActivity;
	private Context mCtxt;
	private DbAlertHandler mDbAlert;
	private String mHiveId;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;

	static private String uniqueIdentifier(String base, String hiveId) {
		return base+"|"+hiveId;
	}
	
	public static boolean isRateDefined(Context ctxt, String hiveId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		return SP.contains(uniqueIdentifier(SAMPLE_RATE, hiveId)) && 
			!SP.getString(uniqueIdentifier(SAMPLE_RATE, hiveId), Integer.toString(DEFAULT_SAMPLE_RATE)).equals(Integer.toString(DEFAULT_SAMPLE_RATE));
	}
	
	public static int getRate(Context ctxt, String hiveId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String valueStr = SP.getString(uniqueIdentifier(SAMPLE_RATE, hiveId), Integer.toString(DEFAULT_SAMPLE_RATE));
		return Integer.parseInt(valueStr);
	}
	
	public static long getRateTimestamp(Context ctxt, String hiveId) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String valueStr = SP.getString(uniqueIdentifier(SAMPLE_RATE_TIMESTAMP, hiveId), DEFAULT_SAMPLE_RATE_TIMESTAMP);
		return Long.parseLong(valueStr);
	}
	
	public static void resetRate(Context ctxt, String hiveId) {
		setRate(ctxt, hiveId, DEFAULT_SAMPLE_RATE, 0);
	}
	
	public SensorSampleRateProperty(final Activity activity, final TextView tv, ImageButton button, DbAlertHandler _dbAlert) {
		this.mCtxt = activity.getApplicationContext();
		this.mActivity = activity;
		this.mSampleRateTv = tv;
		this.mDbAlert = _dbAlert;
		this.mHiveId = HiveEnv.getHiveAddress(activity, ActiveHiveProperty.getActiveHiveName(activity));

		boolean usingConfigRate = false;
		JSONObject config = UptimeProperty.getEmbeddedConfig(activity, mHiveId);
		if (config.has(EMBEDDED_SAMPLE_RATE_PROPERTY)) {
			try {
				long configTimestamp = Long.parseLong(config.getString(UptimeProperty.EMBEDDED_TIMESTAMP));
				long propertyTimestamp = getRateTimestamp(activity, mHiveId);
				if (configTimestamp >= propertyTimestamp) {
					int seconds = Integer.parseInt(config.getString(EMBEDDED_SAMPLE_RATE_PROPERTY));
					int minute_s = 60;
					int rate_min = seconds/minute_s;
					setRate(mHiveId, rate_min, configTimestamp);
					usingConfigRate = true;
					displayRate(Integer.toString(rate_min));
				}
			} catch (NumberFormatException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (JSONException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
		if (!usingConfigRate) {
			displayRate(Integer.toString(getRate(activity, mHiveId)));
		}
		
		mSampleRateTv.setBackgroundColor(HiveEnv.ModifiableFieldBackgroundColor);
		
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				AlertDialog.Builder builder = new AlertDialog.Builder(activity);
				
				builder.setIcon(R.drawable.ic_hive);
				builder.setView(R.layout.sample_rate_dialog);
				builder.setTitle(R.string.sample_rate_dialog_title);
				builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
		        	@Override
		        	public void onClick(DialogInterface dialog, int which) {
		        		final int rateValue = Integer.parseInt(((EditText)mAlert.findViewById(R.id.sampleRateTextValue)).getText().toString());
		        		final String rateValueStr = Integer.toString(rateValue*60);
		        		
		        		CouchCmdPush.OnCompletion onCompletion = new CouchCmdPush.OnCompletion() {
							@Override
							public void success() {
								mActivity.runOnUiThread(new Runnable() {
									@Override
									public void run() {
						        		setRate(mHiveId, rateValue, System.currentTimeMillis()/1000);
						        		SplashyText.highlightModifiedField(mActivity, mSampleRateTv);
									}
								});
							}
							@Override
							public void error(String query, final String msg) {
								mActivity.runOnUiThread(new Runnable() {
									@Override
									public void run() {
										Runnable cancelAction = new Runnable() {public void run() {mAlert.dismiss(); mAlert = null;}};
										mAlert = DialogUtils.createAndShowErrorDialog(mActivity, msg, android.R.string.cancel, cancelAction);
									}
								});
								ACRA.getErrorReporter().handleException(new Exception(query+" failed with msg: "+msg));
							}
							@Override
							public void serviceUnavailable(final String msg) {
								mDbAlert.informDbInaccessible(mActivity, msg, mSampleRateTv.getId());
							}
						};

						new CouchCmdPush(mCtxt, "sample-rate", rateValueStr, onCompletion).execute();

						mAlert.dismiss(); 
						mAlert = null;
		        	}
		        });
		        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
		        });
		        mAlert = builder.show();
		        
		        EditText tv = (EditText) mAlert.findViewById(R.id.sampleRateTextValue);
		        int t = getRate(mActivity, mHiveId);
		        tv.setText(Integer.toString(t));
			}
		});
	}

	public AlertDialog getAlertDialog() {return mAlert;}

	private void resetRate() {
		mSampleRateTv.setText(Integer.toString(DEFAULT_SAMPLE_RATE));
	}
	
	private void displayRate(String msg) {
		mSampleRateTv.setText(msg);
	}
	
	private void setRate(String hiveId, int rate, long timestamp) {
		setRate(mActivity, hiveId, rate, timestamp);
		displayRate(Integer.toString(rate));
	}
	
	public static void setRate(Context ctxt, String hiveId, int minutes, long timestamp) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(uniqueIdentifier(SAMPLE_RATE, hiveId), Integer.toString(DEFAULT_SAMPLE_RATE)).equals(Integer.toString(minutes))) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(uniqueIdentifier(SAMPLE_RATE, hiveId), Integer.toString(minutes));
			editor.commit();
		}
		if (!SP.getString(uniqueIdentifier(SAMPLE_RATE_TIMESTAMP, hiveId), DEFAULT_SAMPLE_RATE_TIMESTAMP).equals(Long.toString(timestamp))) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(uniqueIdentifier(SAMPLE_RATE_TIMESTAMP, hiveId), Long.toString(timestamp));
			editor.commit();
		}
	}

	@Override
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent) {
		return false;
	}

	@Override
	public void onPermissionResult(int requestCode, String[] permissions,
			int[] grantResults) {
		// intentionally unimplemented
	}
	
}
