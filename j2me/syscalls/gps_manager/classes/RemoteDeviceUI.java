import javax.microedition.lcdui.*;
import javax.bluetooth.*;

/**
*
* <p>Title: Remote Device List Component</p>
* <p>Description: This is a List screen to display a list of discovered Bluetooth
* devices. This is a class used by BLUEletUI.
*
* </p>
* @author Ben Hui (www.benhui.net)
* @version 1.0
*
* LICENSE:
* This code is licensed under GPL. (See http://www.gnu.org/copyleft/gpl.html)
*/
class RemoteDeviceUI extends List
{

  public RemoteDeviceUI()
  {
    super("Bluetooth devices", List.IMPLICIT);

    Command select = new Command( "Select", Command.SCREEN, 1 );
    addCommand(select);
    setSelectCommand(select);
    addCommand( new Command( "Search", Command.SCREEN, 2 ) );
    addCommand( BLUElet.BACK );

    setCommandListener( BLUElet.instance );
  }

  /**
   * Set a one-line message to screen.
   * @param str String
   */
  public void setMsg( String str )
  {
    super.deleteAll();
    append( str, null );

  }
  /**
   * refresh the list with blutooth devices
   */
  public void showui()
  {
    super.deleteAll();

    if (BLUElet.devices.size() > 0)
    {
      for (int i = 0; i < BLUElet.devices.size(); i++)
      {
        try
        {
          RemoteDevice device = (RemoteDevice) BLUElet.devices.elementAt(i);
	  try {
            String name = device.getFriendlyName(false);
            append(name, null);
	  } catch (Exception e) {
	    append("Unknown device", null);
	  }
        } catch (Exception e)
        {
          e.printStackTrace();
        }
      }
    } else
    {
      append(BLUElet.m_not_found_msg, null);
    }
  }

}
