import javax.bluetooth.*;
import javax.microedition.io.StreamConnection;
import javax.microedition.io.Connector;
import javax.microedition.lcdui.*;
import javax.microedition.midlet.*;
import javax.microedition.lcdui.game.*;
import java.lang.*;
import java.util.*;
import java.io.*;

public class GpsManager implements CommandListener, Runnable
{
  private static final int MAX_GPS_INPUT = 100;
  private String url;
  private static GpsManager instance;
  private BLUElet bluelet = null;
  private MIDlet midlet;
  private Vector data = new Vector();

  private GpsManager() {}

  private StreamConnection connection;
  private InputStream reader;
  private static final int LINE_DELIMITER = '\n';
  private Thread runner;
  private boolean connection_err = false;

  public static GpsManager getInstance()
  {
    if (instance == null) {
      instance = new GpsManager();
    }

    return instance;
  }

  public void searchGps(MIDlet m) {
    if (bluelet == null) {
      bluelet = new BLUElet(m, this);
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
    if (url.length() <= 1) url = this.url;
    if (url == null) return -1;

    try {
      connection = (StreamConnection) Connector.open(url, Connector.READ);
      reader = connection.openInputStream();
      connection_err = false;
      return 0;
    }
    catch (Exception e) {
      System.out.println(e.getMessage());
      connection = null;
      return -1;
    }
  }

  /**
   * Closes input stream and bluetooth connection as well as sets the
   * corresponding objects to null.
   */
  public synchronized void disconnect() {
    System.out.println("GPSManager: In disconnect.");
    try {
      synchronized (data) {
        data.removeAllElements();
      }
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
    while (Thread.currentThread() == runner) {
      try {
        byte[] output = new byte[100];
        int count = reader.read(output, 1, output.length-1);
	if (count == -1) {
	  connection_err = true;
	  return;
	}

	output[0] = (byte)count;

        synchronized(data) {
          if (data.size() >= MAX_GPS_INPUT) {
	    data.removeElementAt(0);
            System.err.println("GPSManager: Data overflow.");
	  }
          data.addElement(output);
        }
      }
      catch (IOException ie) {
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
    if (connection == null) return -1;
    if (data.isEmpty()) {
      if (connection_err) {
        disconnect();
	return -1;
      }
      return 0;
    }
    if (size == 0) return 1;

    byte[] bytes;
    synchronized(data) {
      bytes = (byte[]) data.firstElement();
      data.removeElementAt(0);
    }

    int count = bytes[0] & 0xff;

    if (count > size) count = size;
    CRunTime.memcpy(addr, bytes, 1, count);

    return count;

/*
    while (((addr & 0x3) != 0) && (size > 0)) {
      byte b = bytes[count++];
      CRunTime.memoryWriteByte(addr, b);
      addr++;
      size--;
      if ((b == 0) || (size == 0)) return count;
    }

    while (size > 3) {
      int i = 0;
      for (int j=0; j<4; j++) {
        i = i << 8;
        byte b = bytes[count++];
        size--;
        i |= b;
        if (b == 0) {
          i = i << 8*(3-j);
          CRunTime.memoryWriteWord(addr, i);
          return count;
        }
      }

      CRunTime.memoryWriteWord(addr, i);
      addr += 4;
    }

    while (size > 0) {
      byte b = bytes[count++];
      CRunTime.memoryWriteByte(addr, b);
      addr++;
      size--;
      if (b == 0) break;
    }

    return count;
  */
  }
}

