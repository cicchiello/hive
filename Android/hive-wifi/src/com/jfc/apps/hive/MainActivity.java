package com.jfc.apps.hive;

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import org.acra.ACRA;
import org.json.JSONException;
import org.json.JSONObject;

import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.AudioSampler;
import com.jfc.misc.prop.DbCredentialsProperty;
import com.jfc.misc.prop.LatchProperty;
import com.jfc.misc.prop.UptimeProperty;
import com.jfc.srvc.cloud.CouchGetBackground;
import com.jfc.srvc.cloud.PollSensorBackground;
import com.jfc.util.misc.DbAlertHandler;
import com.jfc.util.misc.DialogUtils;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;


public class MainActivity extends Activity {
	private static final String TAG = MainActivity.class.getSimpleName();
	
    private static final int POLL_RATE = 600;
    
    private static final boolean DEBUG = HiveEnv.DEBUG && !HiveEnv.RELEASE_TEST;

    // db polling -- necessary until/unless we have an active notification upon db change
    // the following vars are managed by the startPolling() and cancelPolling() functions
	private AtomicBoolean mStopPoller = new AtomicBoolean(false);
	private AtomicInteger mPollCounter = new AtomicInteger(0);
	private Thread mPollerThread = null;
	private Runnable mPoller = null;
	private MotorProperty m0, m1, m2;
	private LatchProperty latch;
	private UptimeProperty uptime;
	private AudioSampler audio;
	private AlertDialog mAlert;
	private boolean mInitialValuesSet = false;
	
	private DbAlertHandler mDbAlert = null;
	
	private final static int REQUEST_WRITE_EXTERNAL_STORAGE=1;
	
    private void requestPermission(){
            // permission has not been granted yet. Request it directly.
            requestPermissions(
                    new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    REQUEST_WRITE_EXTERNAL_STORAGE);
    }
    
    @Override
    public void onRequestPermissionsResult(int requestCode,String permissions[], int[] grantResults) {
        switch (requestCode) {
            case REQUEST_WRITE_EXTERNAL_STORAGE: {
                if (grantResults.length == 1 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Toast.makeText(this, "permission success", Toast.LENGTH_SHORT).show();

                } else {
                    Toast.makeText(this, "permission failed", Toast.LENGTH_SHORT).show();
                    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
                }
                return;
            }
        }
    }
    
    @Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(DEBUG ? R.layout.hive_main_debug : R.layout.hive_main);
	
		// set initial value(s)
		setInitialValues();
		mInitialValuesSet = true;

		mDbAlert = new DbAlertHandler();
		
    	startPolling();
    	
    	String title = getString(R.string.app_name) + ": "+ActiveHiveProperty.getActiveHiveName(this);
    	setTitle(title);
    	
    }

	private void setInitialValues() {
		String ActiveHive = ActiveHiveProperty.getActiveHiveName(MainActivity.this);
		final String HiveId = HiveEnv.getHiveAddress(MainActivity.this, ActiveHive);
		
		if (DEBUG) {
			((TextView) findViewById(R.id.cpuTempText)).setText("?");
			((TextView) findViewById(R.id.cpuTempTimestampText)).setText("?");
		}
		
		((TextView) findViewById(R.id.tempText)).setText("?");
		((TextView) findViewById(R.id.tempTimestampText)).setText("?");
		
		((TextView) findViewById(R.id.humidText)).setText("?");
		((TextView) findViewById(R.id.humidTimestampText)).setText("?");

		latch = new LatchProperty(this, HiveId, 
								  (TextView) findViewById(R.id.latch_text), 
								  (TextView) findViewById(R.id.latchTimestampText));
		
		m0 = new MotorProperty(this, HiveId, 0, 
							   (TextView) findViewById(R.id.motor0Text), 
							   (ImageButton) findViewById(R.id.selectMotor0Button),
							   (TextView) findViewById(R.id.motor0TimestampText));
		
		m1 = new MotorProperty(this, HiveId, 1, 
							   (TextView) findViewById(R.id.motor1Text), 
							   (ImageButton) findViewById(R.id.selectMotor1Button),
							   (TextView) findViewById(R.id.motor1TimestampText));

		m2 = new MotorProperty(this, HiveId, 2, 
							   (TextView) findViewById(R.id.motor2Text), 
							   (ImageButton) findViewById(R.id.selectMotor2Button),
							   (TextView) findViewById(R.id.motor2TimestampText));

		
		uptime = new UptimeProperty(this, HiveId, 
									(TextView) findViewById(R.id.hiveUptimeText), 
									(ImageButton) findViewById(R.id.selectHiveUptimeButton),
									(TextView) findViewById(R.id.hiveUptimeTimestampText));
		
		audio = new AudioSampler(this, HiveId, 
								 (ImageButton) findViewById(R.id.audioSampleButton), 
								 (TextView) findViewById(R.id.audioSampleText), new DbAlertHandler());
		
		if (DEBUG) {
			HiveEnv.setValue(this, R.id.cpuTempText, R.id.cpuTempTimestampText,
					 MCUTempProperty.getMCUTempValue(this, HiveId), 
					 MCUTempProperty.isMCUTempPropertyDefined(this, HiveId), 
					 MCUTempProperty.getMCUTempDate(this, HiveId));
		}
		
		HiveEnv.setValue(this, R.id.tempText, R.id.tempTimestampText,
				 TempProperty.getTempValue(this, HiveId), 
				 TempProperty.isTempPropertyDefined(this, HiveId), 
				 TempProperty.getTempDate(this, HiveId));
		
		HiveEnv.setValue(this, R.id.humidText, R.id.humidTimestampText,
				 HumidProperty.getHumidValue(this), 
				 HumidProperty.isHumidPropertyDefined(this), 
				 HumidProperty.getHumidDate(this));
		
		HiveEnv.setValue(this, R.id.beecntText, R.id.beecntTimestampText,
				 BeeCntProperty.getBeeCntValue(this), 
				 BeeCntProperty.isBeeCntPropertyDefined(this), 
				 BeeCntProperty.getBeeCntDate(this));
					
		HiveEnv.setValue(this, R.id.hiveUptimeText, R.id.hiveUptimeTimestampText,
				 "TBD", 
				 UptimeProperty.isUptimePropertyDefined(this, HiveId), 
				 UptimeProperty.getUptime(this, HiveId));
		UptimeProperty.display(this, HiveId, R.id.hiveUptimeText, R.id.hiveUptimeTimestampText);
	}

	
	private void startPolling() {
		synchronized (this) {
			if (mPoller == null) {
				mPoller = new Runnable() {
					@Override
					public void run() {
						// check about once a minute whenever this activity is running (it doesn't get
						// updated more frequently)
						mPollCounter.set(POLL_RATE-10); // start nearly full scale so that first poll is soon -- subsequent's will be spaced regularly
						while (!mStopPoller.get()) {
							try {
								Thread.sleep(95);
							} catch (InterruptedException e) {
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
							
							// 95ms*600==57000ms
							if (mPollCounter.getAndIncrement() >= POLL_RATE) {
								mPollCounter.set(0);
								
								String ActiveHive = ActiveHiveProperty.getActiveHiveName(MainActivity.this);
								final String HiveId = HiveEnv.getHiveAddress(MainActivity.this, ActiveHive);
								boolean haveHiveId = HiveId != null;
								
								if (haveHiveId) {
									PollSensorBackground.ResultCallback onCompletion;
									PollSensorBackground.OnSaveValue onSaveValue;
									String dbUrl = DbCredentialsProperty.getCouchLogDbUrl(MainActivity.this);

									if (DEBUG) {
										onSaveValue = new PollSensorBackground.OnSaveValue() {
											@Override
											public void save(Activity activity, String objId, String value, long timestamp) {
												MCUTempProperty.setMCUTempProperty(activity, HiveId, value, timestamp);
												HiveEnv.setValueWithSplash(activity, R.id.cpuTempText, R.id.cpuTempTimestampText, value, true, timestamp);
											}
										};
										onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
												"cputemp", R.id.cpuTempText, mDbAlert, onSaveValue);
							            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "cputemp"), onCompletion).execute();
									}
									
									onSaveValue = new PollSensorBackground.OnSaveValue() {
										@Override
										public void save(Activity activity, String objId, String value, long timestamp) {
									        TempProperty.setTempProperty(activity, HiveId, value, timestamp);
											HiveEnv.setValueWithSplash(activity, R.id.tempText, R.id.tempTimestampText, value, true, timestamp);
										}
									};
									onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
											"temp", R.id.tempText, mDbAlert, onSaveValue);
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "temp"), onCompletion).execute();

									onSaveValue = new PollSensorBackground.OnSaveValue() {
										@Override
										public void save(Activity activity, String objId, String value, long timestamp) {
									        HumidProperty.setHumidProperty(activity, value, timestamp);
											HiveEnv.setValueWithSplash(activity, R.id.humidText, R.id.humidTimestampText, value, true, timestamp);
										}
									};
									onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
											"humid", R.id.humidText, mDbAlert, onSaveValue);
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "humid"), onCompletion).execute();

									onSaveValue = new PollSensorBackground.OnSaveValue() {
										@Override
										public void save(Activity activity, String objId, String value, long timestamp) {
											BeeCntProperty.setBeeCntProperty(activity, value, timestamp);
											HiveEnv.setValueWithSplash(activity, R.id.beecntText, R.id.beecntTimestampText, value, true, timestamp);
										}
									};
									onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
											"beecnt", R.id.beecntText, mDbAlert, onSaveValue);
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "beecnt"), onCompletion).execute();
						            
									onSaveValue = new PollSensorBackground.OnSaveValue() {
										@Override
										public void save(Activity activity, String objId, String value, long timestamp) {
											LatchProperty.setLatchProperty(activity, HiveId, value, timestamp);
											HiveEnv.setValueWithSplash(activity, R.id.latch_text, R.id.latchTimestampText, value, true, timestamp);
										}
									};
									onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
											"latch", R.id.latch_text, mDbAlert, onSaveValue);
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "latch"), onCompletion).execute();


						            m0.pollCloud();
						            m1.pollCloud();
						            m2.pollCloud();
						            
									onSaveValue = new PollSensorBackground.OnSaveValue() {
										@Override
										public void save(Activity activity, String objId, String value, long timestamp) {
											UptimeProperty.setUptimeProperty(activity, HiveId, Long.parseLong(value), timestamp);
											UptimeProperty.display(activity, HiveId, R.id.hiveUptimeText, R.id.hiveUptimeTimestampText);
										}
									};
									onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
											"heartbeat", R.id.hiveUptimeText, mDbAlert, onSaveValue);
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "heartbeat"), onCompletion).execute();
						            
						            
									onSaveValue = new PollSensorBackground.OnSaveValue() {
										@Override
										public void save(final Activity activity, final String objId, String value, final long timestamp) {
											CouchGetBackground.OnCompletion displayer = new CouchGetBackground.OnCompletion() {
												public void complete(JSONObject results) {
													try {
														if (results.has("_attachments")) {
															String attName = results.getJSONObject("_attachments").keys().next();
															AudioSampler.setAttachment(activity, HiveId, objId, attName, timestamp);
														}
														AudioSampler.updateParentUI(activity, HiveId, 
																					R.id.audioSampleText, 
																					R.id.audioSampleTimestampText, 
																					R.id.audioSampleButton);
													} catch (JSONException e) {
														// TODO Auto-generated catch block
														e.printStackTrace();
													}
												}
												public void objNotFound(String query) {failed(query, "Object Not Found (listener)");}
												public void failed(String query, String msg) {
													Toast.makeText(MainActivity.this, msg+"; sending a report to my developer", Toast.LENGTH_LONG).show();
													ACRA.getErrorReporter().handleException(new Exception(msg));												}
											};
											final String dbUrl = DbCredentialsProperty.getCouchLogDbUrl(activity);
											String authToken = null;
											new CouchGetBackground(dbUrl+"/"+objId, authToken, displayer).execute();
										}
									};
									onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
											"listener", 0 /*R.id.audioSampleText*/, mDbAlert, onSaveValue);
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "listener"), onCompletion).execute();
								}
							}
						}
					}
				};
				mStopPoller.set(false);
				mPollerThread = new Thread(mPoller, "CouchPoller");
				mPollerThread.start();
			}
		}
	}
	
	private void cancelPoller() {
		mStopPoller.set(true);
		mPollerThread = null;
		mPoller = null;
	}
	
	@Override
	protected void onPause() {
		cancelPoller();
		
		mDbAlert.onPause(this);
		
		if (mAlert != null)
			mAlert.dismiss();
		
		m0.onPause();
		m1.onPause();
		m2.onPause();
		
		if (audio != null) 
			audio.onPause();
		
		mInitialValuesSet = false;
		
		super.onPause();
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		if (!mInitialValuesSet)
			setInitialValues();
		
		startPolling();
		
    	String title = getString(R.string.app_name) + ": "+ActiveHiveProperty.getActiveHiveName(this);
    	setTitle(title);
    	
    	if (!ActiveHiveProperty.isActiveHivePropertyDefined(this)) {
    		Runnable onCancel = new Runnable() {
				@Override
				public void run() {
					mAlert.dismiss();
					mAlert = null;
					onSettings();
				}
			};
    		mAlert = DialogUtils.createAndShowErrorDialog(this, "You must establish a Hivewiz pairing first", 
    													  android.R.string.ok, onCancel);
    	}
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		mPollCounter.set(POLL_RATE-10); // force the fetch to happen very soon
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			onSettings();
			return true;
		}
		if (id == R.id.action_motor_settings) {
			onMotorSettings();
			return true;
		}
		if (id == R.id.action_about) {
			onAbout();
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
	
	public void onSettings() {
		Intent intent = new Intent(this, HiveSettingsActivity.class);
		startActivityForResult(intent, 0);
	}
	
	public void onMotorSettings() {
		Intent intent = new Intent(this, MotorSettingsActivity.class);
		startActivityForResult(intent, 0);
	}
	
	public void onAbout() {
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		
		builder.setIcon(R.drawable.ic_hive);
		builder.setView(R.layout.about_dialog);
		builder.setTitle(R.string.about_dialog_title);
        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {mAlert.dismiss(); mAlert = null;}
        });
        mAlert = builder.show();
	}
}
