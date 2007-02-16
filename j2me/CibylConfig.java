/**
 * Configuration for the Cibyl application. If you want to edit these
 * settings, copy it to your application directory, edit it and
 * install it into src/ when compiling your application.
 */
class CibylConfig
{
    /** Memory size - 0 means use default size (Runtime.freeMemory() * CibylConfig.cibylMemoryProportion) */
    public static int memorySize = 3500000;

    /** The proportion of memory allocated to Cibyl (0..1) */
    public static float cibylMemoryProportion = 0.7f;

    /** Stack size - 8KB by default */
    public static int stackSize = 8192;
}
