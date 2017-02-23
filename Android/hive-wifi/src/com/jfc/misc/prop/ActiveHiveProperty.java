package com.jfc.misc.prop;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Pair;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;
import android.widget.TextView;

import com.jfc.apps.hive.HiveEnv;
import com.jfc.apps.hive.R;
import com.jfc.util.misc.SplashyText;


public class ActiveHiveProperty implements IPropertyMgr {
	private static final String TAG = ActiveHiveProperty.class.getName();

	private static final String ACTIVE_HIVE_PROPERTY = "ACTIVE_HIVE_PROPERTY";
	private static final String DEFAULT_ACTIVE_HIVE = "???";
	
    static final int grayColor = HiveEnv.ModifiableFieldBackgroundColor;
	
    // created on constructions -- no need to save on pause
	private TextView mActiveHiveTv;
	private Activity mActivity;
	private Context mCtxt;

	// transient variables -- no need to save on pause
	private AlertDialog mAlert;
	private List<Pair<String,String>> mExistingPairs = null;

	
	public static boolean isActiveHivePropertyDefined(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		return SP.contains(ACTIVE_HIVE_PROPERTY) && 
			!SP.getString(ACTIVE_HIVE_PROPERTY, DEFAULT_ACTIVE_HIVE).equals(DEFAULT_ACTIVE_HIVE);
	}
	
	public static int getActiveHiveIndex(Context ctxt) {
		ctxt = ctxt.getApplicationContext();
		String hiveName = getActiveHiveName(ctxt);
		for (int j = 0; j < NumHivesProperty.getNumHivesProperty(ctxt); j++) {
			if (PairedHiveProperty.getPairedHiveName(ctxt, j).equals(hiveName)) {
				return j;
			}
		}
		return -1;
	}
	
	public static String getActiveHiveName(Context ctxt) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		String id = SP.getString(ACTIVE_HIVE_PROPERTY, DEFAULT_ACTIVE_HIVE);
		return id;
	}
	
	public static void resetActiveHiveProperty(Context ctxt) {
		setActiveHiveProperty(ctxt.getApplicationContext(), DEFAULT_ACTIVE_HIVE);
	}
	
	private void loadExistingPairs(Context ctxt)
	{
    	mExistingPairs = new ArrayList<Pair<String,String>>();
    	int sz = NumHivesProperty.getNumHivesProperty(ctxt);
    	for (int i = 0; i < sz; i++) {
    		String name = PairedHiveProperty.getPairedHiveName(ctxt, i);
    		String id = PairedHiveProperty.getPairedHiveId(ctxt, i);
    		mExistingPairs.add(new Pair<String,String>(name,id));
    	}
    	
    	if (mExistingPairs.size() == 1) {
    		setActiveHiveName(mExistingPairs.get(0).first);
    		setActiveHiveProperty(ctxt, mExistingPairs.get(0).first);
    	} else {
    		if (isActiveHivePropertyDefined(ctxt)) {
    			setActiveHiveName(getActiveHiveName(ctxt));
    		} else {
    			setActiveHiveUndefined();
    		}
    	}
	}

	public ActiveHiveProperty(Activity _activity, TextView _idTv, ImageButton button) {
		this.mCtxt = _activity.getApplicationContext();
		this.mActivity = _activity;
		this.mActiveHiveTv = _idTv;
		
		loadExistingPairs(mCtxt);
		
		if (isActiveHivePropertyDefined(mCtxt)) {
    		String hiveName = PairedHiveProperty.getPairedHiveName(mCtxt, ActiveHiveProperty.getActiveHiveIndex(mCtxt));
    		displayPairingState(hiveName);
		} else {
			setActiveHiveUndefined();
		}
    	
    	button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
		    	
				loadExistingPairs(mCtxt);
				
		        final ArrayAdapter<Pair<String,String>> arrayAdapter = new BridgePairingsProperty.PairAdapter(mActivity, mExistingPairs);
		        
		        arrayAdapter.sort(new Comparator<Pair<String,String>>() {
					@Override
					public int compare(Pair<String,String> lhs, Pair<String,String> rhs) {
						return lhs.first.compareTo(rhs.first);
					}
				});
		        
		    	AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
		        builder.setIcon(R.drawable.ic_hive);
		        builder.setTitle(R.string.active_hive_choice);
		        builder.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
		            @Override
		            public void onClick(DialogInterface dialog, int which) {
		            	mAlert.dismiss();
		            	mAlert = null;
		            }
		        });
	            builder.setAdapter(arrayAdapter, new DialogInterface.OnClickListener() {
	                @Override
	                public void onClick(DialogInterface dialog, int which) {
	                	String selectedName = mExistingPairs.get(which).first;
	            		mAlert.dismiss();
	            		mAlert = null;
	            		
	            		setActiveHiveName(selectedName);
                		SplashyText.highlightModifiedField(mActivity, mActiveHiveTv);
	                }
	            });
		        mAlert = builder.show();
			}
		});
	}

	public AlertDialog getAlertDialog() {return mAlert;}

	public void setActiveHiveUndefined() {
		setActiveHiveProperty(mCtxt, DEFAULT_ACTIVE_HIVE);
		mActiveHiveTv.setText(DEFAULT_ACTIVE_HIVE);
		mActiveHiveTv.setBackgroundColor(0xffff0000); // RED
	}
	
	private void displayPairingState(String msg) {
		mActiveHiveTv.setText(msg);
		mActiveHiveTv.setBackgroundColor(grayColor);
	}
	
	public void setActiveHiveName(String msg) {
		setActiveHiveProperty(mCtxt, msg);
		displayPairingState(msg);
	}
	
	public static void setActiveHiveProperty(Context ctxt, String id) {
		SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(ctxt.getApplicationContext());
		if (!SP.getString(ACTIVE_HIVE_PROPERTY, DEFAULT_ACTIVE_HIVE).equals(id)) {
			SharedPreferences.Editor editor = SP.edit();
			editor.putString(ACTIVE_HIVE_PROPERTY, id);
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
