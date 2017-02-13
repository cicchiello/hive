package com.jfc.apps.hive;

import com.jfc.misc.prop.ActiveHiveProperty;
import com.jfc.misc.prop.BridgePairingsProperty;
import com.jfc.misc.prop.HiveFactoryResetProperty;
import com.jfc.misc.prop.IPropertyMgr;
import com.jfc.misc.prop.NumHivesProperty;
import com.jfc.misc.prop.PairedHiveProperty;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

import android.app.ActionBar;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.MenuItem;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

public class WifiSettingsActivity extends Activity {
	private static final String TAG = WifiSettingsActivity.class.getName();

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

        setContentView(R.layout.wifi_settings);

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

        final ActiveHiveProperty activeHive = new ActiveHiveProperty(this, (TextView) findViewById(R.id.active_hive_text), (ImageButton) findViewById(R.id.active_hive_button));
        BridgePairingsProperty pairing = new BridgePairingsProperty(this, (TextView) findViewById(R.id.hive_id_text), (ImageButton) findViewById(R.id.hive_pair_button)) {
        	@Override 
        	public void onChange() {
        		Context ctxt = WifiSettingsActivity.this;
        		if (NumHivesProperty.isNumHivesPropertyDefined(ctxt) && (NumHivesProperty.getNumHivesProperty(ctxt) == 1)) {
        			activeHive.setActiveHive(PairedHiveProperty.getPairedHiveName(ctxt, 0));
        		}
        	}
        };
        mMgrs.add(activeHive);
        mMgrs.add(pairing);

        List<HiveFactoryResetProperty.Resetter> resetters = new ArrayList<HiveFactoryResetProperty.Resetter>();
    	resetters.add(new HiveFactoryResetProperty.Resetter() {
    		public void reset(Context ctxt) {
    			NumHivesProperty.clearNumHivesProperty(ctxt);
    			ActiveHiveProperty.resetActiveHiveProperty(ctxt);
    			BridgePairingsProperty.resetHiveIdProperty(ctxt);
    		}});
        mMgrs.add(new HiveFactoryResetProperty(this, (ImageButton) findViewById(R.id.wifiResetButton), 
        		R.string.wifi_reset_question, resetters));

        setTitle(getString(R.string.app_name)+": BLE Bridge");
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
