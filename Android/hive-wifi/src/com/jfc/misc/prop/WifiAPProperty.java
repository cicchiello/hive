package com.jfc.misc.prop;

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
import android.widget.Toast;

import com.jfc.apps.hive.HiveEnv;
import com.jfc.apps.hive.R;
import com.jfc.srvc.cloud.CouchCmdPush;
import com.jfc.srvc.cloud.CouchGetBackground;
import com.jfc.util.misc.DbAlertHandler;
import com.jfc.util.misc.DialogUtils;
import com.jfc.util.misc.SplashyText;


public class WifiAPProperty implements IPropertyMgr {
	private static final String TAG = WifiAPProperty.class.getName();

	private static final String SSID_PROPERTY = "AP_SSID_PROPERTY";
	private static final String PASSWORD_PROPERTY = "AP_PASSWORD_PROPERTY";
	
	private static final String DEFAULT_SSID = "???";
	private static final String DEFAULT_PASSWORD = "password";

    static final int grayColor = HiveEnv.ModifiableFieldBackgroundColor;
    
    // created on constructions -- no need to save on pause
	private TextView mSsidTv;
	private Activity mActivity;
	private Context mCtxt;
	private DbAlertHandler mDbAlert;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;

	
	public static boolean isAPDefined(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		boolean apIsDefault = SP.getString(SSID_PROPERTY, DEFAULT_SSID).equals(DEFAULT_SSID);
		boolean pswdIsDefault = SP.getString(PASSWORD_PROPERTY, DEFAULT_PASSWORD).equals(DEFAULT_PASSWORD);
		
		return !(apIsDefault && pswdIsDefault);
	}
	
	public static String getSSID(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		return SP.getString(SSID_PROPERTY, DEFAULT_SSID);
	}
	
	public static String getPSWD(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		return SP.getString(PASSWORD_PROPERTY, DEFAULT_PASSWORD);
	}
		
	public static void resetAP(Activity activity) {
		setAP(activity, DEFAULT_SSID, DEFAULT_PASSWORD);
	}
	
	public WifiAPProperty(final Activity activity, final TextView ssidTv, ImageButton button, DbAlertHandler _dbAlert) {
		this.mActivity = activity;
		this.mCtxt = activity.getApplicationContext();
		this.mSsidTv = ssidTv;
		this.mDbAlert = _dbAlert;
		final String hiveId = HiveEnv.getHiveAddress(activity, ActiveHiveProperty.getActiveHiveName(activity));
		
		if (isAPDefined(mCtxt)) {
			displayAP(getSSID(mCtxt), getPSWD(mCtxt));
		} else {
			resetAP();
    	}
		mSsidTv.setBackgroundColor(HiveEnv.ModifiableFieldBackgroundColor);
		
		CouchGetBackground.OnCompletion ssidOnCompletion =new CouchGetBackground.OnCompletion() {
			@Override
			public void objNotFound(String query) {
				failed(query, "Object Not Found (config AP)");
			}
			
			@Override
			public void failed(final String query, final String msg) {
				mActivity.runOnUiThread(new Runnable() {
					public void run() {
						Toast.makeText(mActivity, msg+"; sending a report to my developer", Toast.LENGTH_LONG).show();
						ACRA.getErrorReporter().handleException(new Exception(query+"failed with msg: "+msg));
					}
				});
			}

			@Override
			public void complete(final JSONObject resultDoc) {
				try {
					final String SSID = resultDoc.getString("ssid");
					mActivity.runOnUiThread(new Runnable() {public void run() {mSsidTv.setText(SSID);}});
				} catch (JSONException je) {
					failed("invalid JSON doc: "+resultDoc.toString(), je.getMessage());
				}
			}
		};
		HiveEnv.couchGetConfig(mActivity, hiveId, ssidOnCompletion);
		
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				CouchGetBackground.OnCompletion onCompletion =new CouchGetBackground.OnCompletion() {
					@Override
					public void objNotFound(String query) {
						failed(query, "Object Not Found (config)");
					}
					
					@Override
					public void failed(final String query, final String msg) {
						mActivity.runOnUiThread(new Runnable() {
							public void run() {
								Toast.makeText(mActivity, msg+"; sending a report to my developer", Toast.LENGTH_LONG).show();
								ACRA.getErrorReporter().handleException(new Exception(query+" failed with msg: "+msg));
							}
						});
					}

					@Override
					public void complete(final JSONObject doc) {
						mActivity.runOnUiThread(new Runnable() {public void run() {onDialog(doc);}});
					}
				};
				
				HiveEnv.couchGetConfig(mActivity, hiveId, onCompletion);
			}
		});
	}

	private void onDialog(final JSONObject doc) {
		String originalSsid = null, originalPswd = null;
		try {
			originalSsid = doc.getString("ssid");
			originalPswd = doc.getString("pswd");
		} catch (JSONException je) {
			Toast.makeText(mActivity, "Couldn't parse the JSON doc", Toast.LENGTH_LONG).show();
		}
		
		AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
		
		builder.setIcon(R.drawable.ic_hive);
		builder.setView(R.layout.ap_dialog);
		builder.setTitle(R.string.ap_title);
		builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
        	@Override
        	public void onClick(DialogInterface dialog, int which) {
        		final String newSsid = ((EditText) mAlert.findViewById(R.id.ssid_text)).getText().toString();
        		final String newPswd = ((EditText) mAlert.findViewById(R.id.pswd_text)).getText().toString();

        		CouchCmdPush.OnCompletion onCompletion = new CouchCmdPush.OnCompletion() {
					@Override
					public void success() {
						mActivity.runOnUiThread(new Runnable() {
							@Override
							public void run() {
				        		setAP(newSsid, newPswd);
				        		SplashyText.highlightModifiedField(mActivity, mSsidTv);
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
						mDbAlert.informDbInaccessible(mActivity, msg, mSsidTv.getId());
					}
				};

				new CouchCmdPush(mCtxt, "access-point", newSsid+"|"+newPswd, onCompletion).execute();

				mAlert.dismiss(); 
				mAlert = null;
        	}
        });
        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
        });
        mAlert = builder.show();
        
		((EditText) mAlert.findViewById(R.id.ssid_text)).setText(originalSsid);
		((EditText) mAlert.findViewById(R.id.pswd_text)).setText(originalPswd);
	}
	
	
	public AlertDialog getAlertDialog() {return mAlert;}

	private void resetAP() {
		setAP(DEFAULT_SSID, DEFAULT_PASSWORD);
		mSsidTv.setBackgroundColor(0xffff0000); // RED
	}
	
	private void displayAP(String ssid, String pswd) {
		mSsidTv.setText(ssid);
	}
	
	private void setAP(String ssid, String pswd) {
		setAP(mCtxt, ssid, pswd);
		displayAP(ssid, pswd);
	}
	
	public static void setAP(Context ctxt, String ssid, String pswd) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		boolean somethingChanged = 
				!SP.getString(SSID_PROPERTY, DEFAULT_SSID).equals(ssid) ||
				!SP.getString(PASSWORD_PROPERTY, DEFAULT_PASSWORD).equals(pswd);
		if (somethingChanged) {
			SharedPreferences.Editor editor = SP.edit();
			if (!SP.getString(SSID_PROPERTY, DEFAULT_SSID).equals(ssid)) 
				editor.putString(SSID_PROPERTY, ssid);
			if (!SP.getString(PASSWORD_PROPERTY, DEFAULT_PASSWORD).equals(pswd)) 
				editor.putString(PASSWORD_PROPERTY, pswd);
			editor.commit();
		}
	}
	
	@Override
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent) {
		return false;
	}

	@Override
	public void onPermissionResult(int requestCode, String[] permissions, int[] grantResults) {
	}
	
}
