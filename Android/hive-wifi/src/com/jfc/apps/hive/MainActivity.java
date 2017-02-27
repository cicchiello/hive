package com.jfc.apps.hive;

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import org.json.JSONException;
import org.json.JSONObject;

import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.AudioSampler;
import com.jfc.misc.prop.DbCredentialsProperty;
import com.jfc.misc.prop.LatchProperty;
import com.jfc.misc.prop.UptimeProperty;
import com.jfc.srvc.cloud.CouchGetBackground;
import com.jfc.srvc.cloud.PollSensorBackground;
import com.jfc.srvc.cloud.PollSensorBackground.OnSaveValue;
import com.jfc.util.misc.DbAlertHandler;
import com.jfc.util.misc.DialogUtils;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
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

	private void requestPermission(MainActivity mainActivity) {
		// TODO Auto-generated method stub
	}

	private void setInitialValues() {
		int hiveIndex = ActiveHiveProperty.getActiveHiveIndex(this);
		
		if (DEBUG) {
			((TextView) findViewById(R.id.cpuTempText)).setText("?");
			((TextView) findViewById(R.id.cpuTempTimestampText)).setText("?");
		}
		
		((TextView) findViewById(R.id.tempText)).setText("?");
		((TextView) findViewById(R.id.tempTimestampText)).setText("?");
		
		((TextView) findViewById(R.id.humidText)).setText("?");
		((TextView) findViewById(R.id.humidTimestampText)).setText("?");

		latch = new LatchProperty(this, (TextView) findViewById(R.id.latch_text), 
								  (TextView) findViewById(R.id.latchTimestampText));
		
		m0 = new MotorProperty(this, 0, 
							   (TextView) findViewById(R.id.motor0Text), 
							   (ImageButton) findViewById(R.id.selectMotor0Button),
							   (TextView) findViewById(R.id.motor0TimestampText));
		
		m1 = new MotorProperty(this, 1, 
							   (TextView) findViewById(R.id.motor1Text), 
							   (ImageButton) findViewById(R.id.selectMotor1Button),
							   (TextView) findViewById(R.id.motor1TimestampText));

		m2 = new MotorProperty(this, 2, 
							   (TextView) findViewById(R.id.motor2Text), 
							   (ImageButton) findViewById(R.id.selectMotor2Button),
							   (TextView) findViewById(R.id.motor2TimestampText));

		
		uptime = new UptimeProperty(this,
									(TextView) findViewById(R.id.hiveUptimeText), 
									(ImageButton) findViewById(R.id.selectHiveUptimeButton),
									(TextView) findViewById(R.id.hiveUptimeTimestampText));
		
		audio = new AudioSampler(this, (ImageButton) findViewById(R.id.audioSampleButton), new DbAlertHandler());
		
		if (DEBUG) {
			HiveEnv.setValue(this, R.id.cpuTempText, R.id.cpuTempTimestampText,
					 MCUTempProperty.getMCUTempValue(this), 
					 MCUTempProperty.isMCUTempPropertyDefined(this), 
					 MCUTempProperty.getMCUTempDate(this));
		}
		
		HiveEnv.setValue(this, R.id.tempText, R.id.tempTimestampText,
				 TempProperty.getTempValue(this), 
				 TempProperty.isTempPropertyDefined(this), 
				 TempProperty.getTempDate(this));
		
		HiveEnv.setValue(this, R.id.humidText, R.id.humidTimestampText,
				 HumidProperty.getHumidValue(this), 
				 HumidProperty.isHumidPropertyDefined(this), 
				 HumidProperty.getHumidDate(this));
		
		HiveEnv.setValue(this, R.id.motor0Text, R.id.motor0TimestampText,
				 MotorProperty.getMotorValue(this, 0), 
				 MotorProperty.isMotorPropertyDefined(this, 0), 
				 MotorProperty.getMotorDate(this, 0));
		
		HiveEnv.setValue(this, R.id.motor1Text, R.id.motor1TimestampText,
				 MotorProperty.getMotorValue(this, 1), 
				 MotorProperty.isMotorPropertyDefined(this, 1), 
				 MotorProperty.getMotorDate(this, 1));
		
		HiveEnv.setValue(this, R.id.motor2Text, R.id.motor2TimestampText,
				 MotorProperty.getMotorValue(this, 2), 
				 MotorProperty.isMotorPropertyDefined(this, 2), 
				 MotorProperty.getMotorDate(this, 2));
					
		HiveEnv.setValue(this, R.id.beecntText, R.id.beecntTimestampText,
				 BeeCntProperty.getBeeCntValue(this), 
				 BeeCntProperty.isBeeCntPropertyDefined(this), 
				 BeeCntProperty.getBeeCntDate(this));
					
		HiveEnv.setValue(this, R.id.hiveUptimeText, R.id.hiveUptimeTimestampText,
				 "TBD", 
				 UptimeProperty.isUptimePropertyDefined(this, hiveIndex), 
				 UptimeProperty.getUptime(this, hiveIndex));
		UptimeProperty.display(this, R.id.hiveUptimeText, R.id.hiveUptimeTimestampText);
	}

	private PollSensorBackground.ResultCallback getMotorOnCompletion(final int valueResid, final int timestampResid, final OnSaveValue saver) {
		PollSensorBackground.ResultCallback onCompletion = new PollSensorBackground.ResultCallback() {
			@Override
	    	public void report(final String objId, String sensorType, final String timestampStr, final String stepsStr) {
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						long timestampSeconds = Long.parseLong(timestampStr);
						long timestampMillis = timestampSeconds*1000;
						double linearDistanceMeters = MotorProperty.stepsToLinearDistance(MainActivity.this, Long.parseLong(stepsStr));
						double linearDistanceMillimeters = linearDistanceMeters*1000.0;
						String linearDistanceStr = Long.toString((long)(linearDistanceMillimeters+0.5));
						HiveEnv.setValueWithSplash(MainActivity.this, valueResid, timestampResid, linearDistanceStr, true, timestampMillis);
						saver.save(MainActivity.this, objId, linearDistanceStr, timestampSeconds);
					}
				});
			}
			
			@Override
			public void error(final String msg) {
				String errMsg = "Attempt to get Motor location failed with this message: "+msg;
				mDbAlert.informDbInaccessible(MainActivity.this, errMsg, valueResid);
			}

			@Override
			public void dbAccessibleError(final String msg) {
				mDbAlert.informDbInaccessible(MainActivity.this, msg, valueResid);
			}
		};
		return onCompletion;
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
								final int hiveIndex = ActiveHiveProperty.getActiveHiveIndex(MainActivity.this);
								boolean haveHiveId = HiveId != null;
								
								if (haveHiveId) {
									PollSensorBackground.ResultCallback onCompletion;
									PollSensorBackground.OnSaveValue onSaveValue;
									String dbUrl = DbCredentialsProperty.getCouchLogDbUrl(MainActivity.this);

									if (DEBUG) {
										onSaveValue = new PollSensorBackground.OnSaveValue() {
											@Override
											public void save(Activity activity, String objId, String value, long timestamp) {
												MCUTempProperty.setMCUTempProperty(activity, value, timestamp);
											}
										};
										onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
												"cputemp", R.id.cpuTempText, R.id.cpuTempTimestampText, mDbAlert, onSaveValue);
							            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "cputemp"), onCompletion).execute();
									}
									
									onSaveValue = new PollSensorBackground.OnSaveValue() {
										@Override
										public void save(Activity activity, String objId, String value, long timestamp) {
									        TempProperty.setTempProperty(activity, value, timestamp);
										}
									};
									onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
											"temp", R.id.tempText, R.id.tempTimestampText, mDbAlert, onSaveValue);
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "temp"), onCompletion).execute();

									onSaveValue = new PollSensorBackground.OnSaveValue() {
										@Override
										public void save(Activity activity, String objId, String value, long timestamp) {
									        HumidProperty.setHumidProperty(activity, value, timestamp);
										}
									};
									onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
											"humid", R.id.humidText, R.id.humidTimestampText, mDbAlert, onSaveValue);
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "humid"), onCompletion).execute();

									onSaveValue = new PollSensorBackground.OnSaveValue() {
										@Override
										public void save(Activity activity, String objId, String value, long timestamp) {
											BeeCntProperty.setBeeCntProperty(activity, value, timestamp);
										}
									};
									onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
											"beecnt", R.id.beecntText, R.id.beecntTimestampText, mDbAlert, onSaveValue);
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "beecnt"), onCompletion).execute();
						            
									onSaveValue = new PollSensorBackground.OnSaveValue() {
										@Override
										public void save(Activity activity, String objId, String value, long timestamp) {
											LatchProperty.setLatchProperty(activity, value, timestamp);
										}
									};
									onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
											"latch", R.id.latch_text, R.id.latchTimestampText, mDbAlert, onSaveValue);
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "latch"), onCompletion).execute();


						            
									onCompletion = 
											getMotorOnCompletion(R.id.motor0Text, R.id.motor0TimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String objId, String value, long timestamp) {
													MotorProperty.setMotorProperty(MainActivity.this, 0, value, timestamp);
												}
											});
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "motor0"), onCompletion).execute();

									onCompletion = 
											getMotorOnCompletion(R.id.motor1Text, R.id.motor1TimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String objId, String value, long timestamp) {
													MotorProperty.setMotorProperty(MainActivity.this, 1, value, timestamp);
												}
											});
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "motor1"), onCompletion).execute();
						            
									onCompletion = 
											getMotorOnCompletion(R.id.motor2Text, R.id.motor2TimestampText, new OnSaveValue() {
												@Override
												public void save(Activity activity, String objId, String value, long timestamp) {
													MotorProperty.setMotorProperty(MainActivity.this, 2, value, timestamp);
												}
											});
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "motor2"), onCompletion).execute();

						            
									onSaveValue = new PollSensorBackground.OnSaveValue() {
										@Override
										public void save(Activity activity, String objId, String value, long timestamp) {
											UptimeProperty.setUptimeProperty(activity, hiveIndex, Long.parseLong(value), timestamp);
											UptimeProperty.display(activity, R.id.hiveUptimeText, R.id.hiveUptimeTimestampText);
										}
									};
									onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
											"heartbeat", R.id.hiveUptimeText, R.id.hiveUptimeTimestampText, mDbAlert, onSaveValue);
						            new PollSensorBackground(dbUrl, PollSensorBackground.createQuery(HiveId, "heartbeat"), onCompletion).execute();
						            
						            
									onSaveValue = new PollSensorBackground.OnSaveValue() {
										@Override
										public void save(final Activity activity, final String objId, String value, final long timestamp) {
											CouchGetBackground.OnCompletion displayer = new CouchGetBackground.OnCompletion() {
												public void complete(JSONObject results) {
													try {
														if (results.has("_attachments")) {
															String attName = results.getJSONObject("_attachments").keys().next();
															AudioSampler.setAttachment(activity, hiveIndex, objId, attName, timestamp);
														}
														AudioSampler.display(activity, R.id.audioSampleText, R.id.audioSampleTimestampText);
													} catch (JSONException e) {
														// TODO Auto-generated catch block
														e.printStackTrace();
													}
												}
												public void failed(String msg) {
													Toast.makeText(MainActivity.this, msg, Toast.LENGTH_LONG).show();
												}
											};
											final String dbUrl = DbCredentialsProperty.getCouchLogDbUrl(activity);
											String authToken = null;
											new CouchGetBackground(dbUrl+"/"+objId, authToken, displayer).execute();
										}
									};
									onCompletion = PollSensorBackground.getSensorOnCompletion(MainActivity.this, 
											"listener", 0 /*R.id.audioSampleText*/, R.id.audioSampleTimestampText, mDbAlert, onSaveValue);
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
		
		if (audio != null) 
			audio.onPause();
		
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
		return super.onOptionsItemSelected(item);
	}
	
	public void onSettings() {
		Intent intent = new Intent(this, HiveSettingsActivity.class);
		startActivityForResult(intent, 0);
	}
	
	public void onWifiSettings() {
		Intent intent = new Intent(this, WifiSettingsActivity.class);
		startActivityForResult(intent, 0);
	}
	
	public void onMotorSettings() {
		Intent intent = new Intent(this, MotorSettingsActivity.class);
		startActivityForResult(intent, 0);
	}
}