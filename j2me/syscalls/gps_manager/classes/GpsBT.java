import javax.bluetooth.*;
import javax.microedition.io.StreamConnection;
import javax.microedition.io.Connector;
import javax.microedition.lcdui.*;
import javax.microedition.midlet.*;
import javax.microedition.lcdui.game.*;
import java.lang.*;
import java.util.*;
import java.io.*;

public class GpsBT implements CommandListener, Runnable, GpsIntr
{
  private static final int MAX_GPS_INPUT = 20;
  private String url = "";
  private static GpsBT instance;
  private BLUElet bluelet = null;
  private MIDlet midlet;
  private byte[][] data = new byte[MAX_GPS_INPUT][];
  private int data_head = 0;
  private int data_tail = 0;

  private GpsBT() {}

  private StreamConnection connection;
  private InputStream reader;
  private static final int LINE_DELIMITER = '\n';
  private Thread runner;
  private boolean connection_err = false;

  public static GpsBT getInstance()
  {
    if (instance == null) {
      instance = new GpsBT();
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

  public void searchGps(MIDlet m, String wait_msg, String not_found_msg) {
    if (bluelet == null) {
      bluelet = new BLUElet(m, this, wait_msg, not_found_msg);
      this.midlet = m;
    }

    bluelet.startApp();

    GameCanvas gc = (GameCanvas)CRunTime.getRegisteredObject(
                       Syscalls.NOPH_GameCanvas_get());

    bluelet.startInquiry(DiscoveryAgent.GIAC, new UUID[] {new UUID(0x1101)});
    Display.getDisplay(m).setCurrent(bluelet.getUI());
  }

  public void commandAction(Command cmd, Displayable d) {
    if (cmd.equals(BLUElet.SELECTED)) {
      System.out.println("In BLUElet.SELECTED.");
      // handle event that user has selected a device for service discovery
      // in this example, we ignore it
    }
    else if (cmd.equals(BLUElet.BACK)) {
      // handle event that user has press Back button on Bluetooth Devices screen
      // usually, we switch back to host application screen
      bluelet.pauseApp();
      GameCanvas gc = (GameCanvas)CRunTime.getRegisteredObject(
          Syscalls.NOPH_GameCanvas_get());

      Display.getDisplay(midlet).setCurrent(gc);
    }
    else if (cmd.equals(BLUElet.COMPLETED)) {
      System.out.println("In BLUElet.COMPLETED.");
      // handle event that service discovery is completed
      // usually we pass control back to host MIDlet and connect
      // to remote service for perform network communication
      //Display.getDisplay(m).setCurrent(screen);

      ServiceRecord r = bluelet.getFirstDiscoveredService();

      url = r.getConnectionURL(ServiceRecord.NOAUTHENTICATE_NOENCRYPT, false);
      bluelet.pauseApp();

      System.out.println("GPS URL:" + url);

      GameCanvas gc = (GameCanvas)CRunTime.getRegisteredObject(
          Syscalls.NOPH_GameCanvas_get());

      Display.getDisplay(midlet).setCurrent(gc);
    }
  }

  public synchronized int connect(String url) {
    this.url = url;

    return 0;
  }

  /**
   * Closes input stream and bluetooth connection as well as sets the
   * corresponding objects to null.
   */
  public synchronized void disconnect() {
    System.out.println("GPSManager: In disconnect.");
    stop();
    try {
      data_head = 0;
      data_tail = 0;
      if (reader != null)
        reader.close();
      if (connection != null)
        connection.close();
    } catch (Exception e) {
      // Ignore.
    }
    reader = null;
    connection_err = false;
    connection = null;
    System.out.println("GPSManager: After disconnect.");
  }


  /**
   * Reads in records sent by the GPS receiver. When a supported record has
   * been received pauses for specified amount of time. Continues on I/O
   * errors.
   */
  public void run() {
    try {
      connection = (StreamConnection) Connector.open(this.url, Connector.READ);
      if (connection != null) {
        reader = connection.openInputStream();
      }
    }
    catch (Exception e) {
      System.out.println(e.getMessage());
      return;
    }

    if (Thread.currentThread() != runner) return;
    if (connection == null) {
       // Sleep to avoid a busy loop as RoadMap will try to reconnect
       // as soon as a connection error is discovered.
       try {
          Thread.sleep(2000);
       } catch (InterruptedException e) {}
       connection_err = true;
       return;
    }

    while (Thread.currentThread() == runner) {
      try {
        byte[] output;
        if (((data_head + 1) % MAX_GPS_INPUT) == data_tail) {
	  synchronized (data[data_tail]) {
            if (((data_head + 1) % MAX_GPS_INPUT) == data_tail) {
              data_tail = (data_tail + 1) % MAX_GPS_INPUT;
              System.err.println("GPSManager: Data overflow.");
	    }
	  }
        }
	if (data[data_head] == null) data[data_head] = new byte[100];
	output = data[data_head];
        int count = reader.read(output, 1, output.length-1);
        if (Thread.currentThread() != runner) return;
        if (count == -1) {
          connection_err = true;
          return;
        }

        output[0] = (byte)count;

        data_head = (data_head + 1) % MAX_GPS_INPUT;
      }
      catch (IOException ie) {
        if (Thread.currentThread() != runner) return;
        ie.printStackTrace();
        connection_err = true;
        return;
      }
    }
  }

  /**
   * Starts receving of data (if not yet started).
   *  
   */
  public void start() {
    if (runner == null) {
      runner = new Thread(this);
      runner.start();
    }
  }

  /**
   * Stops receiving of data and disconnects from bluetooth device.
   *  
   */
  public void stop() {
    runner = null;
  }

  public int read(int addr, int size) {
    if (data_head == data_tail) {
      if (connection_err) {
        return -1;
      }
      return 0;
    }
    if (size == 0) return 1;

    byte[] bytes = (byte[]) data[data_tail];
    synchronized (bytes) {
      data_tail = (data_tail + 1) % MAX_GPS_INPUT;

      int count = bytes[0] & 0xff;

      if (count > size) count = size;
      CRunTime.memcpy(addr, bytes, 1, count);

      return count;
    }
  }
}

