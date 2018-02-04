/*
  Graphlcd Addon for Kodi
  Copyright (C) 2016 Manuel Reimer <manuel.reimer@gmx.de>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// This is the C(++) part of the Addon. For performance reasons this uses the
// Python C API and is loaded as binary module into the Python process.
// It does some basic interfacing between Python and graphlcd-base.

// !!!!!!!!!!!
// ! WARNING ! This module is not thread-safe! *ONLY* run in one single thread!
// !!!!!!!!!!!                                               ===

#include <Python.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>

#include <string>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/font.h>
#include <glcddrivers/config.h>
#include <glcddrivers/driver.h>
#include <glcddrivers/drivers.h>
#include <glcdskin/parser.h>
#include <glcdskin/skin.h>
#include <glcdskin/config.h>
#include <glcdskin/string.h>

// Hardcoded path to the global configuration file. This should be delivered
// by graphlcd-base.
static const char* kDefaultConfigFile = "/etc/graphlcd.conf";

// Some global variables to keep some runtime values between function calls
// This is to keep as much graphlcd logic as possible on the "C-side" to keep
// the Python side as simple as possible.
GLCD::cDriver* gLcd = NULL;
GLCD::cSkin* gSkin = NULL;
static PyObject* gGetTokenCallback = NULL;
std::string gResourcePath;
std::string gSkinsPath;
std::string gSkinName;

//
// Skin parser callback class
//

class cMySkinConfig : public GLCD::cSkinConfig
{
private:
    GLCD::cDriver* mDriver;
public:
    cMySkinConfig(GLCD::cDriver* Driver);
    virtual std::string SkinPath(void);
    virtual std::string FontPath(void);
    virtual std::string CharSet(void);
    virtual std::string Translate(const std::string & Text);
    virtual GLCD::cType GetToken(const GLCD::tSkinToken & Token);
    virtual GLCD::cDriver* GetDriver(void) const { return mDriver; }
};

cMySkinConfig::cMySkinConfig(GLCD::cDriver* Driver)
:   mDriver(Driver)
{
}

std::string cMySkinConfig::SkinPath(void) {
  return gSkinsPath + "/" + gSkinName;
}

std::string cMySkinConfig::FontPath(void) {
  return gResourcePath + "/fonts";
}

std::string cMySkinConfig::CharSet(void) {
    // Kodi uses UTF-8 internally even if the system runs on another charset
    // So hardcoding UTF-8 here is OK in case of a Kodi addon
    return "UTF-8";
}

std::string cMySkinConfig::Translate(const std::string & Text) {
  return Text;
}

// Here all token requests from graphlcd are arriving
GLCD::cType cMySkinConfig::GetToken(const GLCD::tSkinToken & Token) {
  // Handle some tokens directly without passing to Python
  if (Token.Name == "ForegroundColor") {
    char buffer[12];
    snprintf(buffer, 11, "0x%08x", gLcd->GetForegroundColor(false));
    return buffer;
  }
  else if (Token.Name == "BackgroundColor") {
    char buffer[12];
    snprintf(buffer, 11, "0x%08x", gLcd->GetBackgroundColor(false));
    return buffer;
  }
  else if (Token.Name == "DefaultForegroundColor") {
    char buffer[12];
    snprintf(buffer, 11, "0x%08x", gLcd->GetForegroundColor(true));
    return buffer;
  }
  else if (Token.Name == "DefaultBackgroundColor") {
    char buffer[12];
    snprintf(buffer, 11, "0x%08x", gLcd->GetBackgroundColor(true));
    return buffer;
  }
  else if (Token.Name == "ScreenWidth")
    return gLcd->Width();
  else if (Token.Name == "ScreenHeight")
    return gLcd->Height();
  else if (Token.Name == "ResourcePath")
    return gResourcePath;
  else if (Token.Name == "")
    return 0;

  // Token not handled, so far. So send this request over to the globally stored
  // Python callback function (gGetTokenCallback).
  PyObject* arglist;
  PyObject* result;
  arglist = Py_BuildValue("(ssii)", Token.Name.c_str(), Token.Attrib.Text.c_str(), Token.Index, Token.MaxItems);
  result = PyObject_CallObject(gGetTokenCallback, arglist);
  Py_DECREF(arglist);
  if (result == NULL)
    return "ERR";

  // We support two types of Python return value. Check for them and do proper
  // type conversion, so graphlcd can handle them.
  if (PyString_Check(result)) {
    std::string returnstring;
    returnstring.assign(PyString_AsString(result));
    Py_DECREF(result);
    return returnstring;
  }
  else if (PyInt_Check(result)) {
    int returninteger = PyInt_AsLong(result);
    Py_DECREF(result);
    return returninteger;
  }

  // Something went wrong. Free result value and send "ERR" string.
  Py_DECREF(result);
  return "ERR";
}

cMySkinConfig* gSkinConfig = NULL;

//////////////////////////////////////////////////////////
// Exported functions to be called by Python start here //
//////////////////////////////////////////////////////////

extern "C" {
  // Loads configuration file.
  // Parameters:
  //   Optional string value with config file path. Falls back to default.
  // Return: None
  static PyObject*
  graphlcd_configload(PyObject* self, PyObject* args) {
    const char* path = kDefaultConfigFile;
    if (!PyArg_ParseTuple(args, "|s", &path))
      return NULL;

    if (GLCD::Config.Load(path) != true) {
      PyErr_SetString(PyExc_IOError, "Failed to read graphlcd.conf");
      return NULL;
    }

    return Py_BuildValue("");
  }

  // This function is used to tell the "C part" about the resource path.
  // Parameters:
  //   One string which is meant to be the full (absolute) resource path
  // Return: None
  static PyObject*
  graphlcd_setresourcepath(PyObject* self, PyObject* args) {
    const char* path;

    if (!PyArg_ParseTuple(args, "s", &path))
      return NULL;

    gResourcePath.assign(path);
    gSkinsPath = gResourcePath + "/skins";

    return Py_BuildValue("i", 1);
  }

  // Loads the driver for the display given by display driver name.
  // Parameters:
  //   One string representing the driver name
  // Return: None
  static PyObject*
  graphlcd_createdriver(PyObject* self, PyObject* args) {
    const char* displayname;

    if (!PyArg_ParseTuple(args, "s", &displayname))
      return NULL;

    int displayNumber = -1;
    for (unsigned int index = 0; index < GLCD::Config.driverConfigs.size(); index++) {
      if (GLCD::Config.driverConfigs[index].name == displayname) {
        displayNumber = index;
        break;
      }
    }

    if (displayNumber == -1) {
      PyErr_SetString(PyExc_NameError, "No such display in graphlcd.conf");
      return NULL;
    }

    if (gLcd)
      gLcd->DeInit();
    delete gLcd;

    gLcd = GLCD::CreateDriver(GLCD::Config.driverConfigs[displayNumber].id,
                              &GLCD::Config.driverConfigs[displayNumber]);
    if (!gLcd)
      return NULL;

    // This happens if wrong LCD type is selected in settings or if the LCD
    // is not connected/detected when Kodi starts
    if (gLcd->Init() != 0) {
      gLcd->DeInit();
      delete gLcd;
      gLcd = NULL;
      PyErr_SetString(PyExc_IOError, "Failed to access LCD");
      return NULL;
    }

    // Start with a cleared display
    gLcd->Clear();
    gLcd->Refresh(true);

    return Py_BuildValue("");
  }

  // Actually parses skin XML file. The skin is expected in the resource dir.
  // Parameters:
  //   One string with skin name
  // Return: None
  static PyObject*
  graphlcd_parseskin(PyObject* self, PyObject* args) {
    const char* name;

    if (!PyArg_ParseTuple(args, "s", &name))
      return NULL;

    std::string path = gSkinsPath + "/" + name + "/" + name + ".skin";
    gSkinName.assign(name);

    if (!gLcd)
      return NULL;

    // Check if path exists and is a file. Graphlcd segfaults otherwise!
    struct stat s;
    if (stat(path.c_str(), &s) != 0 || !S_ISREG(s.st_mode)) {
      PyErr_SetString(PyExc_IOError, "Skin file does not exist");
      return NULL;
    }

    delete gSkinConfig;
    gSkinConfig = new cMySkinConfig(gLcd);

    std::string errorString = "";
    delete gSkin;
    gSkin = GLCD::XmlParse(*gSkinConfig, "test", path, errorString);
    if (!gSkin) {
      PyErr_SetString(PyExc_SyntaxError, "Parsing of Skin failed");
      fprintf(stderr, "GLCD Parser error: %s\n", errorString.c_str());
      return NULL;
    }

    gSkin->SetBaseSize(gLcd->Width(), gLcd->Height());
    return Py_BuildValue("");
  }

  // Renders the given Skin part
  // Parameters:
  //   One string representing main screen name (rendered first)
  //   One string representing the overlay (rendered above main screen)
  //   One python object which has to be the python side token callback
  // Return: None
  static PyObject*
  graphlcd_render(PyObject* self, PyObject* args) {
    const char* screenname;
    const char* overlayname;

    if (!PyArg_ParseTuple(args, "ssO", &screenname, &overlayname, &gGetTokenCallback))
      return NULL;

    if (!PyCallable_Check(gGetTokenCallback))
      return NULL;

    if (!gLcd || !gSkin)
      return NULL;

    GLCD::cBitmap screen(gLcd->Width(), gLcd->Height());
    screen.Clear();

    GLCD::cSkinDisplay* display = gSkin->GetDisplay(screenname);
    if (!display)
      return NULL;
    display->Render(&screen); // void

    if (strlen(overlayname) != 0) {
      display = gSkin->GetDisplay(overlayname);
      if (!display)
        return NULL;
      display->Render(&screen);
    }

    gLcd->SetScreen(screen.Data(), screen.Width(), screen.Height()); // void
    gLcd->Refresh(false); // void

    return Py_BuildValue("");
  }

  // Sets display brightness between 0 and 100
  // Parameters:
  //   One integer with wanted screen brightness in percent (0 to 100)
  // Return: None
  static PyObject*
  graphlcd_setbrightness(PyObject* self, PyObject* args) {
    unsigned int brightness;

    if (!PyArg_ParseTuple(args, "I", &brightness))
      return NULL;

    if (brightness > 100)
      return NULL;

    if (!gLcd)
      return NULL;

    gLcd->SetBrightness(brightness);
    return Py_BuildValue("");
  }

  // De-initializes graphlcd, frees used memory and resets pointers
  // Parameters: none
  // Return: None
  static PyObject*
  graphlcd_shutdown(PyObject* self, PyObject* args) {
    if (gLcd)
      gLcd->DeInit();
    delete gLcd;
    gLcd = NULL;
    delete gSkin;
    gSkin = NULL;
    delete gSkinConfig;
    gSkinConfig = NULL;
    return Py_BuildValue("");
  }

  static PyMethodDef GraphlcdMethods[] = {
    {"ConfigLoad",  graphlcd_configload, METH_VARARGS, "Load config file"},
    {"SetResourcePath", graphlcd_setresourcepath, METH_VARARGS, "Set resource path"},
    {"CreateDriver", graphlcd_createdriver, METH_VARARGS, "Create lcd driver"},
    {"ParseSkin", graphlcd_parseskin, METH_VARARGS, "Parse XML skin file"},
    {"Render", graphlcd_render, METH_VARARGS, "Render given screen"},
    {"SetBrightness", graphlcd_setbrightness, METH_VARARGS, "Set brightness"},
    {"Shutdown", graphlcd_shutdown, METH_NOARGS, "Shutdown graphlcd"},

    {NULL, NULL, 0, NULL}        /* Sentinel */
  };

  PyMODINIT_FUNC
  initgraphlcd(void) {
    (void) Py_InitModule("graphlcd", GraphlcdMethods);
  }
}
