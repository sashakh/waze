import javax.microedition.lcdui.*;
import javax.microedition.midlet.*;
import javax.microedition.lcdui.game.*;
import java.lang.*;
import java.util.*;
import java.io.*;

public class GpsManager implements CommandListener
{

  private GpsIntr gps;
  private static GpsManager instance;
  private MIDlet midlet;
  private String wait_msg;
  private String not_found_msg;
  private String internal_str = "Internal GPS";
  private String external_str = "External GPS";
  private List menu;

  private GpsManager() {}

  public static GpsManager getInstance()
  {
    if (instance == null) {
      instance = new GpsManager();
    }

    return instance;
  }

  public int getURL(int addr, int size) {
    if (gps == null) return -1;

    return gps.getURL(addr, size);
  }

  public void commandAction (Command c, Displayable d) {
    if (c == List.SELECT_COMMAND) {
     if (menu.getSelectedIndex() == 0) gps = GpsLocation.getInstance();
     else gps = GpsBT.getInstance();

     gps.searchGps(midlet, wait_msg, not_found_msg);
    }
  }

  public void setTypeMsgs(String internal, String external) {
    internal_str = internal;
    external_str = external;
  }

  /**
   * Checks whether Location API is supported.
   * 
   * @return a boolean indicating is Location API supported.
   */
  public static boolean isLocationApiSupported()
  {
      String version = System.getProperty("microedition.location.version");
      return (version != null && !version.equals("")) ? true : false;
  }

  public void searchGps(MIDlet m, String wait_msg, String not_found_msg) {
      this.midlet = m;
      this.wait_msg = wait_msg;
      this.not_found_msg = not_found_msg;

      if (isLocationApiSupported()) {
        if (menu == null) {
           menu = new List("GPS", Choice.IMPLICIT);
           menu.append(internal_str, null);
           menu.append(external_str, null);
           //menu.addCommand(exitCommand);
           menu.setCommandListener(this);
        }

        Display.getDisplay(m).setCurrent(menu);

      } else {
        gps = GpsBT.getInstance();
        gps.searchGps(m, wait_msg, not_found_msg);
      }
  }


  public synchronized int connect(String url) {
    if ((url == null) || (url.length() == 0)) return -1;

    System.out.println("URL: " + url);
    if (gps == null) {
      if (isLocationApiSupported() && (url.indexOf(':') == -1)) {
          gps = GpsLocation.getInstance();
      } else {
        gps = GpsBT.getInstance();
      }
    }

    return gps.connect(url);
  }

  /**
   * Closes input stream and bluetooth connection as well as sets the
   * corresponding objects to null.
   */
  public synchronized void disconnect() {
    if (gps != null) gps.disconnect();
  }


  /**
   * Starts receving of data (if not yet started).
   *  
   */
  public void start() {

     if (gps != null) gps.start();
  }

  /**
   * Stops receiving of data and disconnects from bluetooth device.
   *  
   */
  public void stop() {
    if (gps != null) gps.stop();
  }


  public int read(int addr, int size) {
    if (gps == null) return -1;

    return gps.read(addr, size);
  }

}

