package com.jfc.misc.prop;

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

import com.jfc.apps.hive.HiveEnv;
import com.jfc.apps.hive.R;
import com.jfc.util.misc.SplashyText;


public class DbCredentialsProperty implements IPropertyMgr {
	private static final String TAG = DbCredentialsProperty.class.getName();

	public static String CouchDbUrl(String cloudantUser, String dbName) {
		return "http://"+cloudantUser+".cloudant.com/"+dbName;
	}

	private static final String CLOUDANT_USER_PROPERTY = "CLOUDANT_USER_PROPERTY";
	private static final String DB_NAME_PROPERTY = "DB_NAME_PROPERTY";
	private static final String DB_KEY_PROPERTY = "DB_KEY_PROPERTY";
	private static final String DB_PSWD_PROPERTY = "DB_PWD_PROPERTY";
	
	private static final String DEFAULT_CLOUDANT_USER = "jfcenterprises";
	private static final String DEFAULT_DB_NAME = "hive-sensor-log";
	private static final String DEFAULT_DB_KEY = "afteptsecumbehisomorther";
	private static final String DEFAULT_DB_PSWD = "e4f286be1eef534f1cddd6240ed0133b968b1c9a";

    static final int grayColor = HiveEnv.ModifiableFieldBackgroundColor;
	
    // created on constructions -- no need to save on pause
	private TextView mCloudantTv;
	private Activity mActivity;
	private Context mCtxt;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;

	
	public static boolean isDbDefined(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		boolean cloudantIsDefault = 
				SP.getString(CLOUDANT_USER_PROPERTY, DEFAULT_CLOUDANT_USER).equals(DEFAULT_CLOUDANT_USER);
		boolean dbKeyIsDefault = 
				SP.getString(DB_KEY_PROPERTY, DEFAULT_DB_KEY).equals(DEFAULT_DB_KEY);
		boolean dbPswdIsDefault = 
				SP.getString(DB_PSWD_PROPERTY, DEFAULT_DB_PSWD).equals(DEFAULT_DB_PSWD);
		boolean dbNameIsDefault = 
				SP.getString(DB_NAME_PROPERTY, DEFAULT_DB_NAME).equals(DEFAULT_DB_NAME);
		
		boolean allDefaults = cloudantIsDefault && dbKeyIsDefault && dbPswdIsDefault && dbNameIsDefault;
		return !allDefaults;
	}
	
	public static String getCloudantUser(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String key = SP.getString(CLOUDANT_USER_PROPERTY, DEFAULT_CLOUDANT_USER);
		return key;
	}
	
	public static String getDbName(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String key = SP.getString(DB_NAME_PROPERTY, DEFAULT_DB_NAME);
		return key;
	}
	
	public static String getDbKey(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String key = SP.getString(DB_KEY_PROPERTY, DEFAULT_DB_KEY);
		return key;
	}
	
	public static String getDbPswd(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt);
		String key = SP.getString(DB_PSWD_PROPERTY, DEFAULT_DB_PSWD);
		return key;
	}
	
	public static void resetDb(Activity activity) {
		setDbCredentials(activity, DEFAULT_CLOUDANT_USER, DEFAULT_DB_NAME, DEFAULT_DB_KEY, DEFAULT_DB_PSWD);
	}
	
	public DbCredentialsProperty(final Activity activity, final TextView keyTv, ImageButton button) {
		this.mActivity = activity;
		this.mCtxt = activity.getApplicationContext();
		this.mCloudantTv = keyTv;
		
		if (isDbDefined(mCtxt)) {
			displayDbCredentials(getCloudantUser(mCtxt), getDbName(mCtxt), getDbKey(mCtxt), getDbPswd(mCtxt));
		} else {
			resetDb();
    	}
		mCloudantTv.setBackgroundColor(HiveEnv.ModifiableFieldBackgroundColor);
		
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
				
				builder.setIcon(R.drawable.ic_hive);
				builder.setView(R.layout.db_credentials_dialog);
				builder.setTitle(R.string.db_credentials_title);
				builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
		        	@Override
		        	public void onClick(DialogInterface dialog, int which) {
		        		String nkey = ((EditText)mAlert.findViewById(R.id.db_key_text)).getText().toString();
		        		String npswd = ((EditText)mAlert.findViewById(R.id.db_pswd_text)).getText().toString();
		        		String ncloudantUser = ((EditText)mAlert.findViewById(R.id.cloudant_user_text)).getText().toString();
		        		
		        		setDbCredentials(ncloudantUser, DEFAULT_DB_NAME, nkey, npswd);

		        		SplashyText.highlightModifiedField(mActivity, mCloudantTv);
						mAlert.dismiss(); 
						mAlert = null;
		        	}
		        });
		        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
		        });
		        mAlert = builder.show();
		        
		        EditText keyTv = (EditText) mAlert.findViewById(R.id.db_key_text);
		        EditText pswdTv = (EditText) mAlert.findViewById(R.id.db_pswd_text);
		        EditText cloudantUserTv = (EditText) mAlert.findViewById(R.id.cloudant_user_text);
		        keyTv.setText(getDbKey(mCtxt));
		        pswdTv.setText(getDbPswd(mCtxt));
		        cloudantUserTv.setText(getCloudantUser(mCtxt));
			}
		});
	}

	public AlertDialog getAlertDialog() {return mAlert;}

	private void resetDb() {
		setDbCredentials(mCtxt, DEFAULT_CLOUDANT_USER, DEFAULT_DB_NAME, DEFAULT_DB_KEY, DEFAULT_DB_PSWD);
		displayDbCredentials(DEFAULT_CLOUDANT_USER, DEFAULT_DB_NAME, DEFAULT_DB_KEY, DEFAULT_DB_PSWD);
		//mCloudantTv.setBackgroundColor(0xffff0000); // RED
	}
	
	private void displayDbCredentials(String cloudantUser, String dbName, String key, String pswd) {
		mCloudantTv.setText(cloudantUser);
	}
	
	private void setDbCredentials(String cloudantUser, String dbName, String key, String pswd) {
		setDbCredentials(mCtxt, cloudantUser, dbName, key, pswd);
		displayDbCredentials(cloudantUser, dbName, key, pswd);
	}
	
	public static void setDbCredentials(Context ctxt, String cloudantUser, String dbName, String key, String pswd) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		boolean somethingChanged = 
				!SP.getString(CLOUDANT_USER_PROPERTY, DEFAULT_CLOUDANT_USER).equals(cloudantUser) ||
				!SP.getString(DB_KEY_PROPERTY, DEFAULT_DB_KEY).equals(key) ||
				!SP.getString(DB_PSWD_PROPERTY, DEFAULT_DB_PSWD).equals(pswd) ||
				!SP.getString(DB_NAME_PROPERTY, DEFAULT_DB_NAME).equals(dbName);
		if (somethingChanged) {
			SharedPreferences.Editor editor = SP.edit();
			if (!SP.getString(CLOUDANT_USER_PROPERTY, DEFAULT_CLOUDANT_USER).equals(cloudantUser)) {
				editor.putString(CLOUDANT_USER_PROPERTY, cloudantUser);
			}
			if (!SP.getString(DB_KEY_PROPERTY, DEFAULT_DB_KEY).equals(key)) {
				editor.putString(DB_KEY_PROPERTY, key);
			}
			if (!SP.getString(DB_PSWD_PROPERTY, DEFAULT_DB_PSWD).equals(pswd)) {
				editor.putString(DB_PSWD_PROPERTY, pswd);
			}
			if (!SP.getString(DB_NAME_PROPERTY, DEFAULT_DB_NAME).equals(dbName)) {
				editor.putString(DB_NAME_PROPERTY, dbName);
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
