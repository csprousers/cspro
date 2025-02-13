package gov.census.cspro.engine;

import java.util.List;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.location.LocationManager;
import android.net.wifi.WifiManager;
import android.os.BatteryManager;
import android.os.Build;
import android.os.SystemClock;
import android.provider.Settings;
import android.telephony.*;
import android.util.DisplayMetrics;


public class ParadataDeviceQueryRunner
{
	private static final String CalculationErrorText = "Calculation Error";

    public static void DeviceInfoQuery(Activity activity,String[] values)
    {
		DisplayMetrics metrics = null;

		for( int i = 0; i < values.length; i++ )
		{
			try
			{
				switch( i )
				{
					case 0: // screen_width
					{
						metrics = new DisplayMetrics();
						activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
						values[i] = String.valueOf(metrics.widthPixels);
						break;
					}

					case 1: // screen_height
					{
						values[i] = String.valueOf(metrics.heightPixels);
						break;
					}

					case 2: // screen_inches
					{
						// "one DIP is one pixel on an approximately 160 dpi screen"
						double density = metrics.density * 160;
						double widthInchesSquared = Math.pow(metrics.widthPixels / density,2);
						double heightInchesSquared = Math.pow(metrics.heightPixels / density,2);
						double screenInches = Math.sqrt(widthInchesSquared + heightInchesSquared);
						values[i] = String.valueOf(screenInches);
						break;
					}

					case 3: // memory_ram
					{
						ActivityManager activityManager = (ActivityManager)activity.getSystemService(Context.ACTIVITY_SERVICE);
						ActivityManager.MemoryInfo memoryInfo = new ActivityManager.MemoryInfo();
						activityManager.getMemoryInfo(memoryInfo);
						values[i] = String.valueOf(memoryInfo.totalMem);
						break;
					}

					case 4: // battery_capacity
					{
						// from https://stackoverflow.com/questions/22243461/android-is-there-anyway-to-get-battery-capacity-of-a-device-in-mah
						final String POWER_PROFILE_CLASS = "com.android.internal.os.PowerProfile";
						Object powerProfile = Class.forName(POWER_PROFILE_CLASS).getConstructor(Context.class).newInstance(activity);
						double batteryCapacity = (double)Class
								.forName(POWER_PROFILE_CLASS)
								.getMethod("getAveragePower",String.class)
								.invoke(powerProfile,"battery.capacity");
						values[i] = String.valueOf(batteryCapacity);
						break;
					}

					case 5: // device_brand
					{
						values[i] = Build.BRAND;
						break;
					}

					case 6: // device_device
					{
						values[i] = Build.DEVICE;
						break;
					}

					case 7: // device_hardware
					{
						values[i] = Build.HARDWARE;
						break;
					}

					case 8: // device_manufacturer
					{
						values[i] = Build.MANUFACTURER;
						break;
					}

					case 9: // device_model
					{
						values[i] = Build.MODEL;
						break;
					}

					case 10: // device_processor
					{
						values[i] = Build.SUPPORTED_ABIS[0];
						break;
					}

					case 11: // device_product
					{
						values[i] = Build.PRODUCT;
						break;
					}
				
					default:
						throw new Exception();
				}
            }

			catch( Exception exception )
			{
				values[i] = CalculationErrorText;
			}
		}
    }
    

	public static void ApplicationInstanceQuery(Activity activity,String[] values)
	{
		try
		{
			values[0] = Long.toString(SystemClock.elapsedRealtime());
		}

		catch( Exception exception )
		{
			values[0] = CalculationErrorText;
		}
	}


	private static String BooleanToString(boolean value)
	{
		return value ? "1" : "0";
	}
	
	public static void DeviceStateQuery(Activity activity,String[] values)
	{
		ContentResolver contentResolver = null;
		boolean wifiEnabled = false;
		TelephonyManager telephonyManager = null;
		Intent batteryStatus = null;

		for( int i = 0; i < values.length; i++ )
		{
			try
			{
				switch( i )
				{
					case 0: // bluetooth_enabled
					{
						contentResolver = activity.getContentResolver();
						boolean bBluetoothEnabled = ( Settings.Global.getInt(contentResolver,Settings.Global.BLUETOOTH_ON,0) != 0 );
						values[i] = BooleanToString(bBluetoothEnabled);
						break;
					}

					case 1: // gps_enabled
					{
						LocationManager locationManager = (LocationManager)activity.getSystemService(Context.LOCATION_SERVICE);
						values[i] = BooleanToString(( locationManager != null ) && locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER));
						break;
					}

					case 2: // wifi_enabled
					{
						if( contentResolver != null )
						{
							wifiEnabled = ( Settings.Global.getInt(contentResolver,Settings.Global.WIFI_ON,0) != 0 );
							values[i] = BooleanToString(wifiEnabled);
						}

						break;
					}

					case 3: // wifi_ssid_text
					{
						if( wifiEnabled )
						{
							WifiManager wifiManager = (WifiManager)activity.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
							if( wifiManager != null )
								values[i] = wifiManager.getConnectionInfo().getSSID();
						}

						break;
					}

					case 4: // mobile_network_enabled
					{
						telephonyManager = (TelephonyManager)activity.getSystemService(Context.TELEPHONY_SERVICE);
						values[i] = BooleanToString(telephonyManager != null);
						break;
					}

					case 5: // mobile_network_type
					{
						if( telephonyManager != null )
						{
							// from https://stackoverflow.com/questions/9283765/how-to-determine-if-network-type-is-2g-3g-or-4g
							switch( telephonyManager.getNetworkType() )
							{
								case TelephonyManager.NETWORK_TYPE_GPRS:
								case TelephonyManager.NETWORK_TYPE_EDGE:
								case TelephonyManager.NETWORK_TYPE_CDMA:
								case TelephonyManager.NETWORK_TYPE_1xRTT:
								case TelephonyManager.NETWORK_TYPE_IDEN:
									values[i] = "2G";
									break;
								case TelephonyManager.NETWORK_TYPE_UMTS:
								case TelephonyManager.NETWORK_TYPE_EVDO_0:
								case TelephonyManager.NETWORK_TYPE_EVDO_A:
								case TelephonyManager.NETWORK_TYPE_HSDPA:
								case TelephonyManager.NETWORK_TYPE_HSUPA:
								case TelephonyManager.NETWORK_TYPE_HSPA:
								case TelephonyManager.NETWORK_TYPE_EVDO_B:
								case TelephonyManager.NETWORK_TYPE_EHRPD:
								case TelephonyManager.NETWORK_TYPE_HSPAP:
									values[i] = "3G";
									break;
								case TelephonyManager.NETWORK_TYPE_LTE:
									values[i] = "4G";
									break;
								default:
									values[i] = "Unknown";
									break;
							}
						}

						break;
					}

					case 6: // mobile_network_name_text
					{
						if( telephonyManager != null )
							values[i] = telephonyManager.getNetworkOperatorName();
						break;
					}

					case 7: // mobile_network_strength
					{
						if( telephonyManager != null )
						{
							List<CellInfo> cellInfos = telephonyManager.getAllCellInfo();

							if( ( cellInfos != null ) && ( cellInfos.size() > 0 ) )
							{
								CellInfo cellInfo = cellInfos.get(0);

								if( cellInfo.isRegistered() )
								{
									CellSignalStrength cellSignalStrength = null;

									if( cellInfo instanceof CellInfoCdma )
										cellSignalStrength = ((CellInfoCdma)cellInfo).getCellSignalStrength();

									else if( cellInfo instanceof CellInfoGsm )
										cellSignalStrength = ((CellInfoGsm)cellInfo).getCellSignalStrength();

									else if( cellInfo instanceof CellInfoLte )
										cellSignalStrength = ((CellInfoLte)cellInfo).getCellSignalStrength();

									else if( cellInfo instanceof CellInfoWcdma )
										cellSignalStrength = ((CellInfoWcdma)cellInfo).getCellSignalStrength();

									if( cellSignalStrength != null )
										values[i] = Double.toString(cellSignalStrength.getLevel() / 4.0);
								}
							}							
						}

						break;
					}

					case 8: // battery_level
					{
						IntentFilter intentFilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
						batteryStatus = activity.getApplicationContext().registerReceiver(null,intentFilter);

						if( batteryStatus != null )
						{
							int batteryLevel = batteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL,Integer.MIN_VALUE);
							int batteryScale = batteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE,Integer.MIN_VALUE);

							if( ( batteryLevel >= 0 ) && ( batteryScale >= 0 ) )
								values[i] = Double.toString(100.0 * batteryLevel / batteryScale);
						}

						break;
					}

					case 9: // battery_charging
					{
						if( batteryStatus != null )
						{
							int batteryCharging = batteryStatus.getIntExtra(BatteryManager.EXTRA_STATUS,Integer.MIN_VALUE);

							if( batteryCharging != Integer.MIN_VALUE )
							{
								values[i] = BooleanToString(( batteryCharging == BatteryManager.BATTERY_STATUS_CHARGING ) ||
									( batteryCharging == BatteryManager.BATTERY_STATUS_FULL ));
							}
						}

						break;
					}

					case 10: // screen_brightness
					{
						if( contentResolver != null )
						{
							int brightness = Settings.System.getInt(contentResolver,Settings.System.SCREEN_BRIGHTNESS);
							values[i] = Double.toString(100.0 * brightness / 255);
						}

						break;
					}

					case 11: // screen_orientation_portrait
					{
						values[i] = BooleanToString(activity.getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT);
						break;
					}

					default:
						throw new Exception();
				}
            }

			catch( Exception exception )
			{
			}
		}
	}
}
