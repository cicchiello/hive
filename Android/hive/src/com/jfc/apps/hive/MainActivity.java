package com.jfc.apps.hive;

import java.util.Calendar;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;

import com.example.hive.R;
import com.jfc.util.misc.SplashyText;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.format.DateFormat;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;

public class MainActivity extends Activity {
	private static final String TAG = MainActivity.class.getSimpleName();
	
	private static final String HiveId = "6402766935504d51202020351c0914ff";
    private static final String DbHost = "http://192.168.1.85";
    private static final int DbPort = 5984;
    private static final String Db = "hive-sensor-log";

    // db polling -- necessary until/unless we have an active notification upon db change
    // the following vars are managed by the startPolling() and cancelPolling() functions
	private AtomicBoolean mStopPoller = new AtomicBoolean(false);
	private Thread mPollerThread = null;
	private Runnable mPoller = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.hive_main);
	
		// set initial value(s)
		setInitialValues();

    	startPolling();
	}

	private void setInitialValues() {
		String v = MCUTempProperty.getMCUTempValue(this);
	    TextView cpuTempText = (TextView) findViewById(R.id.cpuTempText);
		cpuTempText.setText(v);
	
	    TextView timestampText = (TextView) findViewById(R.id.timestampText);
		if (MCUTempProperty.isMCUTempPropertyDefined(this)) {
	        Calendar cal = Calendar.getInstance(Locale.ENGLISH);
			cal.setTimeInMillis(MCUTempProperty.getMCUTempDate(this));
			String date = DateFormat.format("dd-MMM-yy HH:mm", cal).toString();
			timestampText.setText(date);
		} else {
			timestampText.setText("<unknown>");
		}
	}

	private void startPolling() {
		synchronized (this) {
			if (mPoller == null) {
				mPoller = new Runnable() {
					@Override
					public void run() {
						// check about once a minute whenever this activity is running (it doesn't get
						// updated more frequently)
						int cnt = 550; // start nearly full scale so that first poll is soon -- subsequent's will be spaced regularly
						while (!mStopPoller.get()) {
							try {
								Thread.sleep(95);
							} catch (InterruptedException e) {
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
							
							// 95ms*600==57000ms
							if (cnt++ == 600) {
								cnt = 0;
								String HiveId = HiveIdProperty.getHiveIdProperty(MainActivity.this);
								HiveId = HiveId.replace(':', '-');
								String rangeStartKeyClause = "[\"" + HiveId + "\",\"cputemp\",\"99999999\"]";
								String rangeEndKeyClause = "[\"" + HiveId + "\",\"cputemp\",\"00000000\"]";
								String query = "_design/SensorLog/_view/by-hive-sensor?endKey=" + rangeEndKeyClause + "&startkey=" + rangeStartKeyClause + "&descending=true&limit=1";
								PollSensorBackground.ResultCallback onCompletion = new PollSensorBackground.ResultCallback() {
									@Override
							    	public void report(String sensorType, final String timestampStr, final String valueStr) {
										runOnUiThread(new Runnable() {
											@Override
											public void run() {
										        TextView timestampText = (TextView) findViewById(R.id.timestampText);
										        long timestamp = Long.parseLong(timestampStr)*1000;
										        MCUTempProperty.setMCUTempProperty(MainActivity.this, valueStr, timestamp);
		
										        Calendar cal = Calendar.getInstance(Locale.ENGLISH);
										        cal.setTimeInMillis(timestamp);
										        String date = DateFormat.format("dd-MMM-yy HH:mm", cal).toString();
		//								        if (!date.equals(timestampText.getText())) {
										        	timestampText.setText(date);
										        	SplashyText.highlightModifiedField(MainActivity.this, timestampText);
		//								        }
										        
										        TextView cpuTempText = (TextView) findViewById(R.id.cpuTempText);
		//								        if (!valueStr.equals(cpuTempText.getText())) {
										        	cpuTempText.setText(valueStr);
										        	SplashyText.highlightModifiedField(MainActivity.this, cpuTempText);
		//								        }
											}
										});
									}
									
									@Override
									public void error(String msg) {
										Log.e(TAG, "Error: "+msg);
									}
								};
					            new PollSensorBackground(DbHost, DbPort, Db, query, onCompletion).execute();
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
		
		super.onPause();
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		setInitialValues();
		startPolling();
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
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
		return super.onOptionsItemSelected(item);
	}
	
	public void onSettings() {
		Intent intent = new Intent(this, HiveSettingsActivity.class);
		startActivityForResult(intent, 0);
	}
	
}
