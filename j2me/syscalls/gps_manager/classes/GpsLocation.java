import java.lang.*;
import java.util.*;
import java.io.*;
import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.microedition.lcdui.game.*;

import javax.microedition.location.Location;
import javax.microedition.location.LocationException;
import javax.microedition.location.LocationListener;
import javax.microedition.location.LocationProvider;
import javax.microedition.location.QualifiedCoordinates;
import javax.microedition.location.Criteria;


public class GpsLocation implements LocationListener, GpsIntr
{
  private class GpsData {
    int time;
    int longitude;
    int latitude;
    int speed;
    int azymuth;
    int horizontal_accuracy;
    int vertical_accuracy;
    int status;
  };

  private static final int MAX_GPS_INPUT = 20;
  private String url = "";
  private static GpsLocation instance;
  private MIDlet midlet;
  private GpsData[] data = new GpsData[MAX_GPS_INPUT];
  private int data_head = 0;
  private int data_tail = 0;

  private GpsLocation() {}

  private LocationProvider provider;


  public static GpsLocation getInstance()
  {
    if (instance == null) {
      instance = new GpsLocation();
    }

    return instance;
  }

  public int getURL(int addr, int size) {
    int len = url.length();
    size--;
    if ((len == 0) || (len > size)) return -1;

    CRunTime.memcpy(addr, url.getBytes(), 0, len);
    CRunTime.memoryWriteByte(addr + len, 0);

    return 0;
  }

  private LocationProvider findProvider() {
    try {
       Criteria cr = new Criteria();
       //cr.setPreferredPowerConsumption(Criteria.POWER_USAGE_HIGH);
       cr.setPreferredPowerConsumption(Criteria.NO_REQUIREMENT);
       //cr.setPreferredPowerConsumption(Criteria.POWER_USAGE_MEDIUM);
       //cr.setPreferredPowerConsumption(Criteria.POWER_USAGE_LOW);
       //cr.setAddressInfoRequired(false);
       //cr.setAltitudeRequired(false);
       //cr.setHorizontalAccuracy(25);
       //cr.setVerticalAccuracy(25);
       //cr.setPreferredResponseTime(500);
       cr.setSpeedAndCourseRequired(true);

       return LocationProvider.getInstance(cr);
    } catch (LocationException e) {
       e.printStackTrace();
    }

    return null;
  }

  private void addGpsData(int status, final Location location)
  {
    if (((data_head + 1) % MAX_GPS_INPUT) == data_tail) {
      synchronized(data[data_tail]) {
        if (((data_head + 1) % MAX_GPS_INPUT) == data_tail) {
          data_tail = (data_tail + 1) % MAX_GPS_INPUT;
	}
      }
      System.err.println("GPSManager: Data overflow.");
    }

    if (data[data_head] == null) data[data_head] = new GpsData();
    GpsData d = data[data_head];

    d.status = status;

    if (status == 'A') {
      QualifiedCoordinates coord = location.getQualifiedCoordinates();
      float speed = location.getSpeed();
      d.time = (int)(location.getTimestamp() / 1000);
      d.longitude = (int)(coord.getLongitude() * 1000000);
      d.latitude = (int)(coord.getLatitude() * 1000000);
      d.speed = (int)speed * 2; // Convert to ~knots
      d.azymuth = (int)location.getCourse();
      d.horizontal_accuracy = (int)coord.getHorizontalAccuracy();
      d.vertical_accuracy = (int)coord.getVerticalAccuracy();
    }

    data_head = (data_head + 1) % MAX_GPS_INPUT;
  }

  public void searchGps(MIDlet m, String wait_msg, String not_found_msg) {
    provider = findProvider();
    if (provider == null)
    {
        Alert msg = new Alert("Error", not_found_msg, null, AlertType.INFO);
        msg.setTimeout(Alert.FOREVER);
        Display.getDisplay(m).setCurrent(msg);
    }

    url = "Internal GPS";

    GameCanvas gc = (GameCanvas)CRunTime.getRegisteredObject(
        Syscalls.NOPH_GameCanvas_get());

    Display.getDisplay(m).setCurrent(gc);
  }

  public int connect(String url) {
    if (provider == null) provider = findProvider();

    if (provider == null) return -1;
    return 0;
  }

  /**
   * Closes input stream and bluetooth connection as well as sets the
   * corresponding objects to null.
   */
  public void disconnect() {
  }


  /**
   * Starts receving of data (if not yet started).
   *  
   */
  public void start() {

     if (provider != null)
     {
         int interval = -1; // default interval of this provider
         int timeout = 0; // parameter has no effect.
         int maxage = 0; // parameter has no effect.

         provider.setLocationListener(this, interval, timeout, maxage);
     }
  }

  /**
   * Stops receiving of data and disconnects from bluetooth device.
   *  
   */
  public void stop() {
     if (provider != null)
     {
         int interval = -1; // default interval of this provider
         int timeout = 0; // parameter has no effect.
         int maxage = 0; // parameter has no effect.

         provider.setLocationListener(null, interval, timeout, maxage);
         data_head = data_tail = 0;
     }
  }


  public int read(int addr, int size) {
    if (data_head == data_tail) {
      return 0;
    }

    if (size == 0) return 1;

    ByteArrayOutputStream bout = new ByteArrayOutputStream();
    DataOutputStream dout = new DataOutputStream( bout );

    GpsData d = data[data_tail];
    synchronized(d) {
      data_tail = (data_tail + 1) % MAX_GPS_INPUT;

      try {
        dout.writeInt(d.time);
        dout.writeInt(d.longitude);
        dout.writeInt(d.latitude);
        dout.writeInt(d.speed);
        dout.writeInt(d.azymuth);
        dout.writeInt(d.horizontal_accuracy);
        dout.writeInt(d.vertical_accuracy);
        dout.writeInt(d.status);
        dout.close();
      } catch (IOException e) {
        e.printStackTrace();
        return -1;
      }
    }

    byte[] bytes = bout.toByteArray();
    CRunTime.memcpy(addr, bytes, 0, bytes.length);

    return 1;
  }


    /**
     * locationUpdated event from LocationListener interface. This method starts
     * a new thread to prevent blocking, because listener method MUST return
     * quickly and should not perform any extensive processing.
     * 
     * Location parameter is set final, so that the anonymous Thread class can
     * access the value.
     */
    
    public void locationUpdated(LocationProvider provider,
            final Location location)
    {
/*
        new Thread()
        {
            public void run()
            {
*/	    
                if (location != null && location.isValid())
                {
                    addGpsData('A', location);
                }
                else
                {
                    addGpsData('V', null);
                }
/*		
            }
        }.start();
*/	
    }

    /**
     * providerStateChanged event from LocationListener interface. This method
     * starts a new thread to prevent blocking, because listener method MUST
     * return quickly and should not perform any extensive processing.
     * 
     * newState parameter is set final, so that the anonymous Thread class can
     * access the value.
     */
    public void providerStateChanged(LocationProvider provider,
            final int newState)
    {
/*	
        new Thread()
        {
            public void run()
            {
*/	    
                switch (newState) {
                    case LocationProvider.AVAILABLE:
                        break;
                    case LocationProvider.OUT_OF_SERVICE:
                        addGpsData('V', null);
                        break;
                    case LocationProvider.TEMPORARILY_UNAVAILABLE:
                        addGpsData('V', null);
                        break;
                    default:
                        break;
                }
/*		
            }
        }.start();
*/	
    }
}
