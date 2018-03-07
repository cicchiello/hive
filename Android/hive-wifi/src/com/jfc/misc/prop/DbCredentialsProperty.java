package com.jfc.misc.prop;

import java.util.Iterator;

import org.acra.ACRA;
import org.json.JSONException;
import org.json.JSONObject;

import android.annotation.SuppressLint;
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

import com.adobe.xmp.impl.Base64;
import com.jfc.apps.hive.HiveEnv;
import com.jfc.apps.hive.R;
import com.jfc.srvc.cloud.CouchGetBackground;
import com.jfc.srvc.cloud.CouchPutBackground;
import com.jfc.util.misc.SplashyText;


public class DbCredentialsProperty implements IPropertyMgr {
	private static final String TAG = DbCredentialsProperty.class.getName();

	//
	// Several static helper functions
	//
	
	public static String CouchDbUrl(String dbHost, String dbName, boolean usesSSL) {
		return (usesSSL ? "https://":"http://") +dbHost+(usesSSL?":443":":5984")+"/"+dbName;
	}

	public static String getCouchLogDbUrl(Context ctxt) {
		return DbCredentialsProperty.CouchDbUrl(getDbHost(ctxt), getLogDbName(ctxt), getDbHostUsesSSL(ctxt));
	}

	public static String getCouchConfigDbUrl(Context ctxt) {
		return DbCredentialsProperty.CouchDbUrl(getDbHost(ctxt), getConfigDbName(ctxt), getDbHostUsesSSL(ctxt));
	}

	public static String getCouchChannelDbUrl(Context ctxt) {
		return DbCredentialsProperty.CouchDbUrl(getDbHost(ctxt), getChannelDbName(ctxt), getDbHostUsesSSL(ctxt));
	}

	public static String getAuthToken(Context ctxt) {
		String key = getDbKey(ctxt);
		return (key==null || key.equals("")) ? null : Base64.encode(key+":"+getDbPswd(ctxt));
	}
	

	private static final String CONFIG_DB_NAME_PROPERTY = "CONFIG_DB_NAME_PROPERTY";
	private static final String LOG_DB_NAME_PROPERTY = "LOG_DB_NAME_PROPERTY";
	private static final String CHANNEL_DB_NAME_PROPERTY = "CHANNEL_DB_NAME_PROPERTY";
	private static final String DB_HOST_PROPERTY = "DB_HOST_PROPERTY";
	private static final String DB_HOST_USES_SSL_PROPERTY = "DB_HOST_USES_SSL_PROPERTY";
	private static final String DB_KEY_PROPERTY = "DB_KEY_PROPERTY";
	private static final String DB_PSWD_PROPERTY = "DB_PWD_PROPERTY";
	
	private static final String DEFAULT_LOG_DB_NAME = "hive-sensor-log";
	private static final String DEFAULT_CONFIG_DB_NAME = "hive-config";
	private static final String DEFAULT_CHANNEL_DB_NAME = "hive-channel";
	private static final String DEFAULT_DB_HOST0_LEGACY = "jfcenterprises.cloudant.com";
	private static final String DEFAULT_DB_HOST0 = "05002446-d325-4075-a688-a1d7a75e2bb8-bluemix.cloudant.com";
	private static final String DEFAULT_DB_KEY0 = "afteptsecumbehisomorther";
	private static final String DEFAULT_DB_PSWD0 = "e4f286be1eef534f1cddd6240ed0133b968b1c9a";
	private static final boolean DEFAULT_USES_SSL0 = true;
	private static final String DEFAULT_DB_HOST1_LEGACY = "hivewiz.cloudant.com";
	private static final String DEFAULT_DB_HOST1 = "f15e7420-c293-43f6-bd2f-a38b0f34b840-bluemix.cloudant.com";
	private static final String DEFAULT_DB_KEY1 = "gromespecorgingeoughtnev";
	private static final String DEFAULT_DB_PSWD1 = "075b14312a355c8563a77bd05c91fe519873fdf4";
	private static final boolean DEFAULT_USES_SSL1 = true;
	private static final String DEFAULT_DB_HOST2 = "192.168.1.85";
	private static final String DEFAULT_DB_KEY2 = "";
	private static final String DEFAULT_DB_PSWD2 = "";
	private static final boolean DEFAULT_USES_SSL2 = false;

    static final int grayColor = HiveEnv.ModifiableFieldBackgroundColor;
    
    // created on constructions -- no need to save on pause
	private TextView mdbHostTv;
	private Activity mActivity;
	private Context mCtxt;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;

	// transitional code
	static boolean migrationDone = false;
	
	private static void migrateLegacyDbHost(Context ctxt) {
		if (!migrationDone) {
			SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
			boolean dbHostIsDefault0Legacy = 
					SP.getString(DB_HOST_PROPERTY, DEFAULT_DB_HOST0_LEGACY).equals(DEFAULT_DB_HOST0_LEGACY);
			boolean dbHostIsDefault1Legacy = 
					SP.getString(DB_HOST_PROPERTY, DEFAULT_DB_HOST0_LEGACY).equals(DEFAULT_DB_HOST1_LEGACY);
			
			if (dbHostIsDefault0Legacy || dbHostIsDefault1Legacy) {
				String migrationDbHost = dbHostIsDefault0Legacy ? DEFAULT_DB_HOST0 : DEFAULT_DB_HOST1;
				boolean migrationUsesSsl = SP.getString(DB_HOST_USES_SSL_PROPERTY, DEFAULT_USES_SSL0?"true":"false").equals(DEFAULT_USES_SSL0?"true":"false");
				String migrationConfigDbName = DEFAULT_CONFIG_DB_NAME;
				String migrationLogDbName = DEFAULT_LOG_DB_NAME;
				String migrationChanDbName = DEFAULT_CHANNEL_DB_NAME;
				String migrationDbKey = SP.getString(DB_KEY_PROPERTY, DEFAULT_DB_KEY0);
				String migrationDbPswd = SP.getString(DB_PSWD_PROPERTY, DEFAULT_DB_PSWD0);
				migrationDone = true; // prevent re-entry
				setAndPushDbCredentials(ctxt, 
						migrationDbHost, migrationUsesSsl, migrationConfigDbName, migrationLogDbName, migrationChanDbName, migrationDbKey, migrationDbPswd, null);
			}
		}
	}
	
	public static boolean isDbDefined(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		boolean dbHostIsDefaultLegacy = 
				SP.getString(DB_HOST_PROPERTY, DEFAULT_DB_HOST0_LEGACY).equals(DEFAULT_DB_HOST0_LEGACY);
		boolean dbHostIsDefault =
				SP.getString(DB_HOST_PROPERTY, DEFAULT_DB_HOST0).equals(DEFAULT_DB_HOST0);
		boolean dbHostUsesSSLIsDefault =
				SP.getString(DB_HOST_USES_SSL_PROPERTY, DEFAULT_USES_SSL0?"true":"false").equals(DEFAULT_USES_SSL0?"true":"false");
		boolean dbKeyIsDefault = 
				SP.getString(DB_KEY_PROPERTY, DEFAULT_DB_KEY0).equals(DEFAULT_DB_KEY0);
		boolean dbPswdIsDefault = 
				SP.getString(DB_PSWD_PROPERTY, DEFAULT_DB_PSWD0).equals(DEFAULT_DB_PSWD0);
		boolean logDbNameIsDefault = 
				SP.getString(LOG_DB_NAME_PROPERTY, DEFAULT_LOG_DB_NAME).equals(DEFAULT_LOG_DB_NAME);
		boolean configDbNameIsDefault = 
				SP.getString(CONFIG_DB_NAME_PROPERTY, DEFAULT_CONFIG_DB_NAME).equals(DEFAULT_CONFIG_DB_NAME);
		boolean channelDbNameIsDefault = 
				SP.getString(CHANNEL_DB_NAME_PROPERTY, DEFAULT_CHANNEL_DB_NAME).equals(DEFAULT_CHANNEL_DB_NAME);
		
		boolean allDefaults = (dbHostIsDefaultLegacy || dbHostIsDefault) && 
				dbHostUsesSSLIsDefault && dbKeyIsDefault && dbPswdIsDefault && 
				logDbNameIsDefault && configDbNameIsDefault && channelDbNameIsDefault;
		
		
		if (dbHostIsDefaultLegacy || SP.getString(DB_HOST_PROPERTY, DEFAULT_DB_HOST1_LEGACY).equals(DEFAULT_DB_HOST1_LEGACY))
			migrateLegacyDbHost(ctxt);
		
		return !allDefaults;
	}

	public static String getDbHost(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String provisionalDb = SP.getString(DB_HOST_PROPERTY, DEFAULT_DB_HOST0);
		if (provisionalDb.equals(DEFAULT_DB_HOST0_LEGACY)) {
			provisionalDb = DEFAULT_DB_HOST0;
			migrateLegacyDbHost(ctxt);
		}
		if (provisionalDb.equals(DEFAULT_DB_HOST1_LEGACY)) {
			provisionalDb = DEFAULT_DB_HOST1;
			migrateLegacyDbHost(ctxt);
		}
		return provisionalDb;
	}
	
	public static boolean getDbHostUsesSSL(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		return SP.getString(DB_HOST_USES_SSL_PROPERTY, DEFAULT_USES_SSL0?"true":"false").equals("true");
	}
	
	public static String getLogDbName(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		return SP.getString(LOG_DB_NAME_PROPERTY, DEFAULT_LOG_DB_NAME);
	}
	
	public static String getConfigDbName(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		return SP.getString(CONFIG_DB_NAME_PROPERTY, DEFAULT_CONFIG_DB_NAME);
	}
	
	public static String getChannelDbName(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		return SP.getString(CHANNEL_DB_NAME_PROPERTY, DEFAULT_CHANNEL_DB_NAME);
	}
	
	public static String getDbKey(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String key = SP.getString(DB_KEY_PROPERTY, DEFAULT_DB_KEY0);
		return key;
	}
	
	public static String getDbPswd(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String key = SP.getString(DB_PSWD_PROPERTY, DEFAULT_DB_PSWD0);
		return key;
	}
	
	public static void resetDb(Activity activity) {
		setDbCredentials(activity, DEFAULT_DB_HOST0, DEFAULT_USES_SSL0, 
				DEFAULT_CONFIG_DB_NAME, DEFAULT_LOG_DB_NAME, DEFAULT_CHANNEL_DB_NAME,
				DEFAULT_DB_KEY0, DEFAULT_DB_PSWD0);
	}
	
	public DbCredentialsProperty(final Activity activity, final TextView keyTv, ImageButton button) {
		this.mActivity = activity;
		this.mCtxt = activity.getApplicationContext();
		this.mdbHostTv = keyTv;
		
		if (isDbDefined(mCtxt)) {
			displayDbCredentials(getDbHost(mCtxt), getDbHostUsesSSL(mCtxt), 
								 	getConfigDbName(mCtxt), getLogDbName(mCtxt), getChannelDbName(mCtxt),
								 	getDbKey(mCtxt), getDbPswd(mCtxt));
		} else {
			resetDb();
    	}
		mdbHostTv.setBackgroundColor(HiveEnv.ModifiableFieldBackgroundColor);
		
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
				
				builder.setIcon(R.drawable.ic_hive);
				builder.setView(R.layout.db_credentials_dialog);
				builder.setTitle(R.string.db_credentials_title);
				builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
		        	@SuppressLint("DefaultLocale")
					@Override
		        	public void onClick(DialogInterface dialog, int which) {
		        		String nDbHost = ((EditText)mAlert.findViewById(R.id.db_host_text)).getText().toString();

		        		String nkey=null, npswd=null;
		        		boolean usesSSL = false;
		        		if (nDbHost.toLowerCase().equals(DEFAULT_DB_HOST0_LEGACY) ||
		        			nDbHost.toLowerCase().equals(DEFAULT_DB_HOST0)) {
		        			nDbHost = DEFAULT_DB_HOST0;
		        			nkey = DEFAULT_DB_KEY0;
		        			npswd = DEFAULT_DB_PSWD0;
		        			usesSSL = DEFAULT_USES_SSL0;
		        		} else if (nDbHost.toLowerCase().equals(DEFAULT_DB_HOST1_LEGACY) ||
		        				   nDbHost.toLowerCase().equals(DEFAULT_DB_HOST1)) {
		        			nDbHost = DEFAULT_DB_HOST1;
		        			nkey = DEFAULT_DB_KEY1;
		        			npswd = DEFAULT_DB_PSWD1;
		        			usesSSL = DEFAULT_USES_SSL1;
		        		} else if (nDbHost.toLowerCase().equals(DEFAULT_DB_HOST2)) {
		        			nDbHost = DEFAULT_DB_HOST2;
		        			nkey = DEFAULT_DB_KEY2;
		        			npswd = DEFAULT_DB_PSWD2;
		        			usesSSL = DEFAULT_USES_SSL2;
		        		}
		        		
		        		setAndPushDbCredentials(mCtxt, nDbHost, usesSSL, DEFAULT_CONFIG_DB_NAME, DEFAULT_LOG_DB_NAME, DEFAULT_CHANNEL_DB_NAME, nkey, npswd, null);

		        		SplashyText.highlightModifiedField(mActivity, mdbHostTv);
						mAlert.dismiss(); 
						mAlert = null;
		        	}
		        });
		        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
		        });
		        mAlert = builder.show();
		        
		        EditText dbHostTv = (EditText) mAlert.findViewById(R.id.db_host_text);
		        dbHostTv.setText(getDbHost(mCtxt));
			}
		});
    	
		migrateLegacyDbHost(activity);
	}

	public AlertDialog getAlertDialog() {return mAlert;}

	private void resetDb() {
		setDbCredentials(mCtxt, DEFAULT_DB_HOST0, DEFAULT_USES_SSL0, 
				DEFAULT_CONFIG_DB_NAME, DEFAULT_LOG_DB_NAME, DEFAULT_CHANNEL_DB_NAME,
				DEFAULT_DB_KEY0, DEFAULT_DB_PSWD0);
		displayDbCredentials(DEFAULT_DB_HOST0, DEFAULT_USES_SSL0, 
				DEFAULT_CONFIG_DB_NAME, DEFAULT_LOG_DB_NAME, DEFAULT_CHANNEL_DB_NAME,
				DEFAULT_DB_KEY0, DEFAULT_DB_PSWD0);
	}
	
	private void displayDbCredentials(String dbHost, boolean usesSSL, 
									  String configDbName, String logDbName, String channelDbName,
									  String key, String pswd) {
		mdbHostTv.setText(dbHost);
	}
	
	private static void setAndPushDbCredentials(final Context ctxt, final String dbHost, final boolean usesSSL, 
										  final String configDbName, final String logDbName, final String channelDbName,
										  final String key, final String pswd, final Runnable onSuccess) {
		
		final String hiveId = PairedHiveProperty.getPairedHiveId(ctxt, ActiveHiveProperty.getActiveHiveIndex(ctxt));
		
		CouchGetBackground.OnCompletion onCompletion =new CouchGetBackground.OnCompletion() {
			@Override
			public void objNotFound(String query) {
				failed(query, "Object Not Found");
			}
			
			@Override
			public void failed(final String query, final String msg) {
				if (ctxt instanceof Activity) {
					final Activity a = (Activity) ctxt;
					a.runOnUiThread(new Runnable() {
						public void run() {
							Toast.makeText(a, msg+"; sending a report to my developer", Toast.LENGTH_LONG).show();
							ACRA.getErrorReporter().handleException(new Exception(query+" failed with msg: "+msg));
						}
					});
				}
				ACRA.getErrorReporter().handleException(new Exception(query+" failed with msg: "+msg));
			}

			@Override
			public void complete(final JSONObject doc) {
        		JSONObject newDoc = new JSONObject();
				try {
	        		Iterator<String> it = doc.keys();
	        		while (it.hasNext()) {
	        			String k = it.next();
	        			if (k.equals("db-host")) {
	        				newDoc.put(k, dbHost);
	        			} else if (k.equals("is-ssl")) {
	        				newDoc.put(k, usesSSL?"true":"false");
	        			} else if (k.equals("db-port")) {
	        				newDoc.put(k, usesSSL?"443":"5984");
	        			} else if (k.equals("db-creds")) {
	        				newDoc.put(k, ((key==null||key.equals(""))?"":Base64.encode(key+":"+pswd)));
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
        		
        		String dbUrl = DbCredentialsProperty.getCouchConfigDbUrl(ctxt)+"/"+hiveId;
    			String authToken = DbCredentialsProperty.getAuthToken(ctxt);
        		
        		CouchPutBackground.OnCompletion onCompletion = new CouchPutBackground.OnCompletion() {
					@Override
					public void complete(JSONObject results) {
						if (ctxt instanceof Activity) {
							final Activity a = (Activity) ctxt;
							setDbCredentials(ctxt, dbHost, usesSSL, configDbName, logDbName, channelDbName, key, pswd);
							a.runOnUiThread(new Runnable() {public void run() {
								if (onSuccess != null)
									onSuccess.run();
								Toast.makeText(a, "Save Successful", Toast.LENGTH_LONG).show();
							}});
						} else {
							@SuppressWarnings("unused")
							String msg = "Save successful";
						}
					}
					@Override
					public void failed(String query, String msg) {
						if (ctxt instanceof Activity) {
							final Activity a = (Activity) ctxt;
							a.runOnUiThread(new Runnable() {public void run() {
								Toast.makeText(a, "Save Failed", Toast.LENGTH_LONG).show();
							}});
						} else {
							@SuppressWarnings("unused")
							String msg2 = "Save failed: "+msg;
						}
						ACRA.getErrorReporter().handleException(new Exception(query+" failed with msg: "+msg));
					}};
        		CouchPutBackground putter = new CouchPutBackground(dbUrl, authToken, newDoc.toString(), onCompletion);
        		putter.execute();
			}
		};
		
		HiveEnv.couchGetConfig(ctxt, hiveId, onCompletion);
	}
	
	public static void setDbCredentials(Context ctxt, String dbHost, boolean usesSSL, 
										String configDbName, String logDbName, String channelDbName, 
										String key, String pswd) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		boolean somethingChanged = 
				!SP.getString(DB_HOST_PROPERTY, DEFAULT_DB_HOST0).equals(dbHost) ||
				!SP.getString(DB_HOST_USES_SSL_PROPERTY, DEFAULT_USES_SSL0?"true":"false").equals(usesSSL) ||
				!SP.getString(DB_KEY_PROPERTY, DEFAULT_DB_KEY0).equals(key) ||
				!SP.getString(DB_PSWD_PROPERTY, DEFAULT_DB_PSWD0).equals(pswd) ||
				!SP.getString(CONFIG_DB_NAME_PROPERTY, DEFAULT_CONFIG_DB_NAME).equals(configDbName) ||
				!SP.getString(LOG_DB_NAME_PROPERTY, DEFAULT_LOG_DB_NAME).equals(logDbName) ||
				!SP.getString(CHANNEL_DB_NAME_PROPERTY, DEFAULT_CHANNEL_DB_NAME).equals(channelDbName);
		if (somethingChanged) {
			SharedPreferences.Editor editor = SP.edit();
			if (!SP.getString(DB_HOST_PROPERTY, DEFAULT_DB_HOST0).equals(dbHost)) {
				editor.putString(DB_HOST_PROPERTY, dbHost);
			}
			if (!SP.getString(DB_HOST_USES_SSL_PROPERTY, DEFAULT_USES_SSL0?"true":"false").equals(usesSSL ? "true":"false")) {
				editor.putString(DB_HOST_USES_SSL_PROPERTY, usesSSL ? "true":"false");
			}
			if (!SP.getString(DB_KEY_PROPERTY, DEFAULT_DB_KEY0).equals(key)) {
				editor.putString(DB_KEY_PROPERTY, key);
			}
			if (!SP.getString(DB_PSWD_PROPERTY, DEFAULT_DB_PSWD0).equals(pswd)) {
				editor.putString(DB_PSWD_PROPERTY, pswd);
			}
			if (!SP.getString(LOG_DB_NAME_PROPERTY, DEFAULT_LOG_DB_NAME).equals(logDbName)) {
				editor.putString(LOG_DB_NAME_PROPERTY, logDbName);
			}
			if (!SP.getString(CONFIG_DB_NAME_PROPERTY, DEFAULT_CONFIG_DB_NAME).equals(configDbName)) {
				editor.putString(CONFIG_DB_NAME_PROPERTY, configDbName);
			}
			if (!SP.getString(CHANNEL_DB_NAME_PROPERTY, DEFAULT_CHANNEL_DB_NAME).equals(channelDbName)) {
				editor.putString(CHANNEL_DB_NAME_PROPERTY, channelDbName);
			}
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
