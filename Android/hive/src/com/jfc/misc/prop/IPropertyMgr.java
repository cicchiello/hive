package com.jfc.misc.prop;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;

public interface IPropertyMgr {
	public AlertDialog getAlertDialog();
	public boolean onActivityResult(int requestCode, int resultCode, Intent intent);
	public void onPermissionResult(int requestCode, String[] permissions, int[] grantResults);
    public void onRestoreInstanceState(Bundle savedInstanceState);
    public void onSaveInstanceState(Bundle savedInstanceState);
}
