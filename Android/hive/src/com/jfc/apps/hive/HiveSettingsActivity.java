package com.jfc.apps.hive;

import com.example.hive.R;
import com.jfc.misc.prop.AcraTestProperty;
import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.BridgePairingsProperty;
import com.jfc.misc.prop.EnableBridgeProperty;
import com.jfc.misc.prop.HiveFactoryResetProperty;
import com.jfc.misc.prop.IPropertyMgr;
import com.jfc.misc.prop.StepsPerRevolutionProperty;
import com.jfc.misc.prop.ThreadsPerMeterProperty;
import com.jfc.util.misc.LocalStorageHandler;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

import android.app.ActionBar;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;

public class HiveSettingsActivity extends Activity {
	private static final String TAG = HiveSettingsActivity.class.getName();

	public static final int SETTINGS_CMD = 35;
	
	private static final boolean DEBUG = HiveEnv.DEBUG;
	private static final boolean RELEASE_TEST = HiveEnv.RELEASE_TEST;
	
	private List<IPropertyMgr> mMgrs = new ArrayList<IPropertyMgr>();
	
    
	@Override
	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Check whether we're recreating a previously destroyed instance
        if (savedInstanceState != null) {
			// activity being re created -- onRestoreInstanceState will be called from parent onStart
			Log.i(TAG, "onCreate; activity being recreated -- onRestoreInstanceState will be called from parent onStart");
        } else {
			Intent i = getIntent();
			if (i == null || (i.getAction() != null && i.getAction().equals("android.intent.action.MAIN"))) {
				// activity being launched directly --- everything defaults to test data
				Log.i(TAG, "onCreate; activity launched directly");
			} else {
				// activity being launched from other activity (normal production case)
				Log.i(TAG, "onCreate; activity launched from other activity");
			}
        }

        setContentView(DEBUG ? R.layout.settings_debug : R.layout.settings);

        try {
        	Method m = getClass().getMethod("getActionBar", (Class<?>[]) null);
	        if (m != null) {
	        	ActionBar bar = (ActionBar) m.invoke(this, null);
	        	m = bar.getClass().getMethod("setDisplayOptions", int.class);
	        	m.invoke(bar, ActionBar.DISPLAY_SHOW_HOME | ActionBar.DISPLAY_SHOW_TITLE);
	        	m = bar.getClass().getMethod("setDisplayHomeAsUpEnabled", boolean.class);
	        	m.invoke(bar, true);
	        }
        } catch (IllegalAccessException iae) {
        	// consume exception -- can't support normal action bar stuff
        } catch (InvocationTargetException ite) {
        	// consume exception -- can't support normal action bar stuff
        } catch (NoSuchMethodException nsme) {
        	// consume exception -- can't support normal action bar stuff
        }

        mMgrs.add(new ActiveHiveProperty(this, (TextView) findViewById(R.id.active_hive_text), (ImageButton) findViewById(R.id.active_hive_button)));
        mMgrs.add(new BridgePairingsProperty(this, (TextView) findViewById(R.id.hive_id_text), (ImageButton) findViewById(R.id.hive_pair_button)));
        mMgrs.add(new EnableBridgeProperty(this, (TextView) findViewById(R.id.enable_bridge_text), (ImageButton) findViewById(R.id.enable_bridge_button)));
        mMgrs.add(new ThreadsPerMeterProperty(this, (TextView) findViewById(R.id.threads_per_meter_text), (ImageButton) findViewById(R.id.threads_per_meter_button)));
        mMgrs.add(new StepsPerRevolutionProperty(this, (TextView) findViewById(R.id.steps_per_rev_text), (ImageButton) findViewById(R.id.steps_per_rev_button)));
        if (DEBUG) 
        	mMgrs.add(new AcraTestProperty(this, (ImageButton) findViewById(R.id.acraTestButton)));

        List<HiveFactoryResetProperty.Resetter> resetters = new ArrayList<HiveFactoryResetProperty.Resetter>();
    	resetters.add(new HiveFactoryResetProperty.Resetter() {public void reset(Activity activity) {BridgePairingsProperty.resetHiveIdProperty(activity);}});
    	resetters.add(new HiveFactoryResetProperty.Resetter() {
    		public void reset(Activity activity) {
        		LocalStorageHandler storage = null;
        		try {
        			storage = new LocalStorageHandler(activity);
        			storage.deleteAll();
        		} finally {
        			if (storage != null) 
        				storage.close();
        		}
    		}});
        mMgrs.add(new HiveFactoryResetProperty(this, (ImageButton) findViewById(R.id.factoryResetButton), resetters));

        setTitle(getString(R.string.app_name)+": Settings");
    }

    @Override
    protected void onPause() {
    	Log.i(TAG, "enterred onPause");
    	
    	for (IPropertyMgr mgr : mMgrs) 
    		if (mgr.getAlertDialog() != null) mgr.getAlertDialog().dismiss();
    	
    	super.onPause();
   		Log.i(TAG, "exitting onPause");
    }
    
    
    @Override
	public void onBackPressed() {
    	Log.i(TAG, "enterred: onBackPressed");
		finish();    	
    	Log.i(TAG, "exitting: onBackPressed");
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
    	if (item.getItemId() == android.R.id.home) {
    		setResult(0);
    		onBackPressed();
    	}

        return true;
    }

	private static final int REQUEST_COARSE_LOCATION=0;

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
    	for (IPropertyMgr mgr : mMgrs) 
    		mgr.onPermissionResult(requestCode, permissions, grantResults);
    }
    
    public void onActivityResult(int requestCode, int resultCode, Intent intent) {
    	for (IPropertyMgr mgr : mMgrs) 
    		if (mgr.onActivityResult(requestCode, resultCode, intent)) {
    		}
    }
    
}
