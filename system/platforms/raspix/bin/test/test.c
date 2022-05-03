#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mmio.h>
#include <uspi.h>
#include <uspios.h>


static void KeyPressedHandler (const char *pString)
{
  klog("%s\n", pString);
}

int main(int argc, char* argv[]) {
	mmio_map(false);
  if (!USPiInitialize ())
  {
    klog("Cannot initialize USPi\n");
    return -1;
  }

  if (!USPiKeyboardAvailable ())
  {
    klog("Keyboard not found\n");
    return -1;
  }

  USPiKeyboardRegisterKeyPressedHandler (KeyPressedHandler);

  klog("Just type something!\n");

  // just wait and turn the rotor
  for (unsigned int nCount = 0; 1; nCount++)
  {
    USPiKeyboardUpdateLEDs ();
  }

  return 0;
}
