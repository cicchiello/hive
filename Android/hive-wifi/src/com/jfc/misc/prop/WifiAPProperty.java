package com.jfc.misc.prop;

import java.util.Iterator;

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
import com.jfc.srvc.cloud.CouchGetBackground;
import com.jfc.srvc.cloud.CouchPutBackground;
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
	
	public WifiAPProperty(final Activity activity, final TextView ssidTv, ImageButton button) {
		this.mActivity = activity;
		this.mCtxt = activity.getApplicationContext();
		this.mSsidTv = ssidTv;
		
		if (isAPDefined(mCtxt)) {
			displayAP(getSSID(mCtxt), getPSWD(mCtxt));
		} else {
			resetAP();
    	}
		mSsidTv.setBackgroundColor(HiveEnv.ModifiableFieldBackgroundColor);
		
		CouchGetBackground.OnCompletion ssidOnCompletion =new CouchGetBackground.OnCompletion() {
			@Override
			public void objNotFound() {
				failed("Object Not Found");
			}
			
			@Override
			public void failed(final String msg) {
				mActivity.runOnUiThread(new Runnable() {public void run() {Toast.makeText(mActivity, msg, Toast.LENGTH_LONG).show();}});
			}

			@Override
			public void complete(final JSONObject resultDoc) {
				try {
					final String SSID = resultDoc.getString("ssid");
					mActivity.runOnUiThread(new Runnable() {public void run() {mSsidTv.setText(SSID);}});
				} catch (JSONException je) {
					failed(je.getMessage());
				}
			}
		};
		HiveEnv.couchGetConfig(mActivity, ssidOnCompletion);
		
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				CouchGetBackground.OnCompletion onCompletion =new CouchGetBackground.OnCompletion() {
					@Override
					public void objNotFound() {
						failed("Object Not Found");
					}
					
					@Override
					public void failed(final String msg) {
						mActivity.runOnUiThread(new Runnable() {
							public void run() {Toast.makeText(mActivity, msg, Toast.LENGTH_LONG).show();}
						});
					}

					@Override
					public void complete(final JSONObject doc) {
						mActivity.runOnUiThread(new Runnable() {public void run() {onDialog(doc);}});
					}
				};
				
				HiveEnv.couchGetConfig(mActivity, onCompletion);
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
        		String newSsid = ((EditText) mAlert.findViewById(R.id.ssid_text)).getText().toString();
        		String newPswd = ((EditText) mAlert.findViewById(R.id.pswd_text)).getText().toString();

        		JSONObject newDoc = new JSONObject();
				try {
	        		Iterator<String> it = doc.keys();
	        		while (it.hasNext()) {
	        			String k = it.next();
	        			if (k.equals("ssid")) {
	        				newDoc.put(k, newSsid);
	        			} else if (k.equals("pswd")) {
	        				newDoc.put(k, newPswd);
	        			} else if (k.equals("timestamp")) {
	        				newDoc.put(k, Long.toString(System.currentTimeMillis()/1000));
	        			} else if (!k.equals("_id")) {
	        				newDoc.put(k, doc.get(k));
	        			}
	        		}
				} catch (JSONException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
        		
        		String hiveId = PairedHiveProperty.getPairedHiveId(mActivity, ActiveHiveProperty.getActiveHiveIndex(mActivity));
        		String dbUrl = DbCredentialsProperty.getCouchConfigDbUrl(mActivity)+"/"+hiveId;
    			String authToken = DbCredentialsProperty.getAuthToken(mCtxt);
        		
        		CouchPutBackground.OnCompletion onCompletion = new CouchPutBackground.OnCompletion() {
					@Override
					public void complete(JSONObject results) {
						mActivity.runOnUiThread(new Runnable() {public void run() {
							Toast.makeText(mActivity, "Save Successful", Toast.LENGTH_LONG).show();
						}});
					}
					@Override
					public void failed(String msg) {
						mActivity.runOnUiThread(new Runnable() {public void run() {
							Toast.makeText(mActivity, "Save Failed", Toast.LENGTH_LONG).show();
						}});
					}};
        		CouchPutBackground putter = new CouchPutBackground(dbUrl, authToken, newDoc.toString(), onCompletion);
        		putter.execute();
        		
        		setAP(newSsid, newPswd);

        		SplashyText.highlightModifiedField(mActivity, mSsidTv);
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
