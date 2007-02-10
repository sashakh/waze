import java.util.*;
import javax.bluetooth.*;
import javax.microedition.lcdui.*;
import javax.microedition.midlet.*;

/**
*
* <p>Title: Bluetooth Application Utility GUI Component</p>
* <p>Description:
*
* Note: This class must be used as singleton.
* </p>
* @author Ben Hui (www.benhui.net)
* @version 1.0
*
* LICENSE:
* This code is licensed under GPL. (See http://www.gnu.org/copyleft/gpl.html)
*/
public class BLUElet  implements CommandListener
{
  // Commands used in callback to idenfity BLUElet events.
  // COMPLETED - When both device and service discovery are completed.
  public static Command COMPLETED =  new Command( "COMPLETED", Command.SCREEN, 1 );
  // SELECTED - When user has selected a Bluetooth device for service search
  public static Command SELECTED =  new Command( "SELECTED", Command.SCREEN, 1 );
  // BACK - When user press Back button on Bluetooth Devices screen (RemoteDeviceUI)
  public static Command BACK = new Command( "Back", Command.BACK, 1 );


  // your MIDlet reference
  public static MIDlet host;
  // your callback CommandListener
  public static CommandListener callback;
  // self instance of BLUEletUI
  public static BLUElet instance;
  // reference to GUI display
  public static Display display;

  public static Vector devices = new Vector();
  public static Vector deviceClasses = new Vector();
  public static Vector services = new Vector();
  public static int selectedDevice = -1;
//  public static int selectedService = -1;


  // discovery mode in device inquiry
  public int discoveryMode;
  // list of UUID to match during service discovery
  public UUID[] serviceUUIDs = null;

  // Bluetooth return code from device inquiry operation
  // see DiscoveryListener
  public int deviceReturnCode;
  // Bluetooth return code from service discovery operation
  // see DiscoveryListener
  public int serviceReturnCode;



  private RemoteDeviceUI remotedeviceui = null;
  private LocalDevice device;
  private DiscoveryAgent agent;


  /**
   * Creae a new BLUElet.
   * @param host MIDlet
   * @param listener CommandListener
   */
  public BLUElet(MIDlet host, CommandListener listener)
  {
    this.host = host;
    this.callback = listener;
    instance = this;
  }

  /**
   * Mirror MIDlet.startApp(), should be called by your MIDlet startApp().
   */
  public void startApp() {

    display = Display.getDisplay(host);

    remotedeviceui = new RemoteDeviceUI();
    remotedeviceui.showui();
  }

  /**
   * Mirror MIDlet.pauseApp(), should be called by your MIDlet pauseApp().
   */
  public void pauseApp()
  {
    // do nothing
  }

  /**
   * Mirror MIDlet.destroyApp(), should be called by your MIDlet destroyApp().
   */
  public void destroyApp(boolean unconditional)
  {
  }


  /**
   * Utility function to write log message.
   * @param s String
   */
  public static void log(String s)
  {
    System.out.println(s);
  }

  /**
   * Obtain reference to device selection screen component.
   * You should show this screen when user invoke device search.
   * @return Screen
   */
  public Screen getUI()
  {
    return remotedeviceui;
  }

  /**
   * Get all discovered services from selected remote device.
   * Your application call this method after your app receive COMPLETED callback
   * event. This will return all services that match your UUIDs in startInquiry().
   * @return ServiceRecord[]
   */
  public ServiceRecord[] getDiscoveredServices()
  {
    ServiceRecord[] r = new ServiceRecord[ services.size() ];
    services.copyInto( r );
    return r;
  }

  /**
   * Get the first discovered service from selected remote device.
   * Your application call this method after your app receives COMPLETED
   * callback event. This will return the first service that match your
   * UUIDs in startInquiry().
   *
   * @return ServiceRecord null if no service discovered
   */
  public ServiceRecord getFirstDiscoveredService()
  {
    if ( services.size() > 0 )
      return (ServiceRecord) services.elementAt(0);
    else
      return null;
  }

  /**
   * Return the Bluetooth result code from device inquiry.
   * This is the result code obtained in  DiscoveryListener.inquiryCompleted().
   * Your application cal call this method after a COMPLETED callback event
   * is received.
   * @return int
   */
  public int getDeviceDiscoveryReturnCode()
  {
    return deviceReturnCode;
  }


  /**
   * Return the Bluetooth result code from service discovery.
   * This is the result code obtained in  DiscoveryListener.serviceSearchCompleted().
   * Your application cal call this method after a COMPLETED callback event
   * is received.
   * @return int
   */
  public int getServiceDiscoveryReturnCode()
  {
    return serviceReturnCode;
  }

  /**
   * Return user selected remote device that is used for service discovery.
   * Your application can call this after your app received SELECTED callback
   * event.
   * @return RemoteDevice null if user didn't select anything
   */
  public RemoteDevice getSelectedDevice()
  {
    if ( selectedDevice != -1 )
      return (RemoteDevice) devices.elementAt(selectedDevice);
    else
      return null;
  }

  /**
   * Start device inquiry. Your application call this method to start inquiry.
   * @param mode int one of DiscoveryAgent.GIAC or DiscoveryAgent.LIAC
   * @param serviceUUIDs UUID[]
   */
  public void startInquiry( int mode, UUID[] serviceUUIDs )
  {
    try
    {
      this.discoveryMode = mode;
      this.serviceUUIDs = serviceUUIDs;

      // clear previous values first
      devices.removeAllElements();
      deviceClasses.removeAllElements();

      //
      // initialize the JABWT stack
      device = LocalDevice.getLocalDevice(); // obtain reference to singleton
      device.setDiscoverable(DiscoveryAgent.GIAC); // set Discover Mode
      agent = device.getDiscoveryAgent(); // obtain reference to singleton


      boolean result = agent.startInquiry( mode, new Listener() );

      // update screen with "Please Wait" message
      remotedeviceui.setMsg("[Please Wait...]");

    } catch ( BluetoothStateException e )
    {
      e.printStackTrace();
    }

  }

  /**
   *
   * @param c Command
   * @param d Displayable
   */
  public void commandAction(Command c, Displayable d)
  {
    if ( d == remotedeviceui && c.getLabel().equals("Search") )
    {
      startInquiry( discoveryMode, serviceUUIDs );

    } else if ( d == remotedeviceui && c.getLabel().equals("Back") )
    {
      callback.commandAction( BACK, remotedeviceui);

    } else if ( d == remotedeviceui && c.getLabel().equals("Select") )
    {
      // get selected device
      selectedDevice = remotedeviceui.getSelectedIndex();
      RemoteDevice remoteDevice = (RemoteDevice) devices.elementAt( selectedDevice );

      // remove all existing record first
      services.removeAllElements();

      try
      {
        agent.searchServices(null,
                             serviceUUIDs,
                             remoteDevice,
                             new Listener() );

        // tell callback device selected
        display.callSerially(new Worker(ID_DEVICE_SELECTED));

      } catch (BluetoothStateException ex)
      {
        ex.printStackTrace();
      }

    }
  }

  /**
   * Bluetooth listener object.
   * Register this listener object to DiscoveryAgent in device inqury and service discovery.
   */
  class Listener implements DiscoveryListener
  {

    public void deviceDiscovered(RemoteDevice remoteDevice,
                                 DeviceClass deviceClass)
    {
      log("A remote Bluetooth device is discovered:");
      //Util.printRemoteDevice( remoteDevice, deviceClass );
      devices.addElement( remoteDevice );
      deviceClasses.addElement( deviceClass );
    }

    public void inquiryCompleted(int complete)
    {
      log("device discovery is completed with return code:"+complete);
      log(""+devices.size()+" devices are discovered");

      deviceReturnCode = complete;

      if ( devices.size() == 0 )
      {
        Alert alert = new Alert( "Bluetooth", "No Bluetooth device found", null, AlertType.INFO );
        alert.setTimeout(3000);
        remotedeviceui.showui();
        display.setCurrent( alert, remotedeviceui );

      } else
      {
        remotedeviceui.showui();
        display.setCurrent( remotedeviceui );
      }

    }

    public void servicesDiscovered(int transId, ServiceRecord[] records)
    {
      // note: we do not use transId because we only have one search at a time
      log("Remote Bluetooth services is discovered:");
      for ( int i=0; i< records.length;  i ++ )
      {
        ServiceRecord record = records[i];
        //Util.printServiceRecord( record );
        services.addElement( record );
      }
    }

    public void serviceSearchCompleted(int transId, int complete)
    {
      // note: we do not use transId because we only have one search at a time
      log("service discovery completed with return code:"+complete);
      log(""+services.size()+" services are discovered");

      serviceReturnCode = complete;

      // we cannot callback in this thread because this is a Bluetooth
      // subsystem thread. we do not want to block it.
      display.callSerially( new Worker( ID_SERVICE_COMPLETED ) );

    }

  } // Listener

  private final static int ID_SERVICE_COMPLETED = 1;
  private final static int ID_DEVICE_COMPLETED = 2;
  private final static int ID_DEVICE_SELECTED = 3;
  /**
   * Worker thread that invoke callback CommandListener upon Bluetooth event occurs.
   */
  class Worker implements Runnable
  {
    int cmd = 0;

    public Worker( int cmd )
    {
      this.cmd = cmd;
    }
    public void run()
    {
      switch (cmd) {
        case ID_SERVICE_COMPLETED:
          callback.commandAction( COMPLETED, remotedeviceui);

          break;
        case ID_DEVICE_COMPLETED:
          callback.commandAction( COMPLETED, remotedeviceui);

          break;
        case ID_DEVICE_SELECTED:
          callback.commandAction( SELECTED, remotedeviceui);

          break;
        default:
          break;

      }
    }
  }

}
