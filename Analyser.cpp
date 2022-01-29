/*
 * Analyser.cpp
 *
 *  Created on: 13.10.2012
 *      Author: Selur
 */

#include "Analyser.h"
#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QApplication>
#include "windows.h"
#include <iostream>

const AVS_Linkage *AVS_linkage = nullptr;

Analyser::Analyser(QObject *parent, const QString& input, bool walk) :
    QObject(parent), m_currentInput(input), m_walk(walk), m_res(0), m_env(nullptr),
    m_avsDLL(this), m_inf(nullptr), m_frameCount(0)
{
  this->setObjectName("Avisynth Analyser");
  QString avisynthDll = QDir::toNativeSeparators(qApp->applicationDirPath() + QDir::separator() + QString("AviSynth.dll"));
  if (!QFile::exists(avisynthDll)) {
    std::cerr << qPrintable(avisynthDll) << " does not exist." << std::endl;
    avisynthDll = QString("AviSynth.dll");
  }
  m_avsDLL.setFileName(avisynthDll);
}

Analyser::~Analyser()
{

}

bool Analyser::loadAvisynthDLL()
{
  if (m_avsDLL.isLoaded()) {
    return true;
  }
  if (!m_avsDLL.load()) {
    QString error = m_avsDLL.errorString();
    if (!error.isEmpty()) {
      std::cerr << "Could not load "<< qPrintable(m_avsDLL.fileName()) <<"! " << std::endl << qPrintable(error) << std::endl;
      return false;
    }
    std::cerr << "Could not load "<< qPrintable(m_avsDLL.fileName()) <<"! " << std::endl;
    return false;
  }
  std::cout << "loaded avisynth dll,..(" << qPrintable(m_avsDLL.fileName()) << ")" << std::endl;
  return true;
}

bool Analyser::initEnv()
{
  //load avisynth.dll if it's not already loaded and abort if it couldn't be loaded
  if (!this->loadAvisynthDLL()) {
    return false;
  }
  // create new script environment
  IScriptEnvironment* (*CreateScriptEnvironment)(int version) = (IScriptEnvironment*(*)(int)) m_avsDLL.resolve("CreateScriptEnvironment"); //resolve CreateScriptEnvironment from the dll
  std::cout << "loaded CreateScriptEnvironment definition from dll,.. " << std::endl;
  try {
    m_env = CreateScriptEnvironment(AVISYNTH_INTERFACE_VERSION); //create a new IScriptEnvironment
    if (!m_env) { //abort if IScriptEnvironment couldn't be created
      std::cerr << "Could not create IScriptenvironment for AVISYNTH_INTERFACE_VERSION " << AVISYNTH_INTERFACE_VERSION <<",..." << std::endl;
      m_env = CreateScriptEnvironment(AVISYNTH_CLASSIC_INTERFACE_VERSION); //create a new IScriptEnvironment
      if (!m_env) { //abort if IScriptEnvironment couldn't be created
        std::cerr << "Could not create IScriptenvironment for AVISYNTH_CLASSIC_INTERFACE_VERSION " << AVISYNTH_CLASSIC_INTERFACE_VERSION <<",..." << std::endl;
        return false;
      } else {
        std::cout << "loaded IScriptEnvironment using AVISYNTH_CLASSIC_INTERFACE_VERSION,.. ("<< AVISYNTH_CLASSIC_INTERFACE_VERSION << ")" << std::endl;
      }
    } else {
      std::cout << "loaded IScriptEnvironment using AVISYNTH_INTERFACE_VERSION,.. ("<< AVISYNTH_INTERFACE_VERSION << ")" << std::endl;
    }
  } catch (AvisynthError &err) { //catch AvisynthErrors
     std::cerr << "-> " << err.msg << std::endl;
     return false;
   } catch (...) { //catch everything else
     std::cerr << "-> CreateScriptEnvironment: Unknown error" << std::endl;
     return false;
   }
  return true;
}

bool Analyser::setRessource()
{
  if (m_env == nullptr) {
    std::cerr << "environment not set in setRessource!" << std::endl;
    return false;
  }
  if (AVS_linkage != nullptr) {
    std::cerr << "AVS_linkage isn't nullptr" << std::endl;
    return false;
  }
  std::cout << "getting avs linkage from environment" << std::endl;
  try {
    AVS_linkage = m_env->GetAVSLinkage();
  } catch (...) { //catch everything else
    std::cerr << "failed getting avs linkage from environment!" << std::endl;
    return false;
  }
  if (AVS_linkage == nullptr) {
    std::cerr << "AVS_linkage is nullptr" << std::endl;
    return false;
  }
  try { // always fails for 32bit WTF?!
    std::cout << "Importing " << qPrintable(m_currentInput) << std::endl;
    const char* infile = m_currentInput.toLocal8Bit(); //convert input name to char*
    AVSValue arg(infile);
    m_res = m_env->Invoke("Import", AVSValue(&arg, 1));
  } catch (AvisynthError &err) { //catch AvisynthErrors
    std::cout << "Failed importing " << qPrintable(m_currentInput) << std::endl;
    std::cerr << "-> " << err.msg << std::endl;
    return false;
  } catch (...) { //catch everything else
    std::cerr << "-> invoking Import for " << qPrintable(m_currentInput) << " failed!" << std::endl;
    return false;
  }
  if (!m_res.IsClip()) {
   std::cerr << "Couldn't load input, not a clip!" << std::endl;
   return false;
  }
  if (!m_res.Defined()) {
    QString error = QObject::tr("Couldn't import:") + " " + m_currentInput;
    error += "\r\n";
    error += QObject::tr("Script seems not to be a valid avisynth script.");
    std::cerr << qPrintable(error) << std::endl;
    return false;
  }
  return true;
}

void Analyser::closing()
{
  if (m_env != nullptr) {
    m_env->Free(&m_res);
    try {
      m_env->DeleteScriptEnvironment(); //delete the old script environment this causes a crash no clue why
    } catch (AvisynthError &err) { //catch AvisynthErrors
      std::cerr << "Failed to delete script environment " << err.msg << std::endl;
    } catch (...) {
      std::cerr << "Failed to delete script environment,.. (Unkown Error)" << std::endl;
    }
    m_env = nullptr; // ensure new environment created next time
  }
  m_res = 0;
  if (m_avsDLL.isLoaded()) {
    m_avsDLL.unload();
  }
  emit closeApplication();
}

bool Analyser::setVideoInfo()
{
  PClip  clip = m_res.AsClip();    //get clip
  m_inf = &(clip->GetVideoInfo());    //get clip infos
  if (!m_inf->HasVideo()) { //abort if clip has no video
    std::cerr << "Input has no video stream -> aborting" << std::endl;
    return false;
  }
  return true;
}

QString Analyser::getColor()
{
  if (m_inf->IsY8()) {
    return QString("Y8");
  }
  if (m_inf->Is420()) {
    return QString("YV12");
  }
  if (m_inf->IsYUY2()) {
    return QString("YUY2");
  }
  if (m_inf->IsYV16()) {
    return QString("YV16");
  }
  if (m_inf->IsYV24()) {
    return QString("YV24");
  }
  if (m_inf->IsRGB24()) {
    return QString("RGB24");
  }
  if (m_inf->IsRGB32()) {
    return QString("RGB32");
  }
  if (m_inf->IsRGB48()) {
    return QString("RGB48");
  }
  if (m_inf->IsRGB()) {
    return QString("RGB");
  }
  if (m_inf->IsYUV()) {
    return QString("YUV");
  }
  return QString("unkown");
}

/**
 * Show the characteristics of the video
 */
void Analyser::showVideoInfo()
{
  std::cout << "Color: " << qPrintable(this->getColor());
  std::cout << ", Resolution: " << m_inf->width << "x" << m_inf->height;
  if (m_inf->fps_denominator == 1) {
     std::cout << ", Frame rate: " << m_inf->fps_numerator << " fps";
  } else {
    std::cout << ", Frame rate: " << m_inf->fps_numerator << "/" << m_inf->fps_denominator << " fps";
  }
  m_frameCount = m_inf->num_frames;
  std::cout << ", Length: " << m_frameCount << " frames";
  if (m_inf->IsBFF()) {
    std::cout << ", BFF" << std::endl;
  } else if (m_inf->IsTFF()) {
    std::cout << ", TFF" << std::endl;
  } else {
    std::cout << ", PRO" << std::endl;
  }
  if (m_inf->HasAudio()) {
    int sampleRate = m_inf->audio_samples_per_second;
    if (sampleRate != 0) {
      std::cout << "Audio:" << std::endl;
      std::cout << "Sample rate: " << sampleRate << " Hz";
      std::cout << ", Channel count: " << m_inf->nchannels << std::endl;
    }
  }
  std::cout << std::endl;
}

void Analyser::analyse()
{
  if (!this->initEnv()) {
    this->closing();
    return;
  }
  if (!this->setRessource()) {
    this->closing();
    return;
  }
  this->setVideoInfo();
  this->showVideoInfo();
  if (m_walk && m_frameCount > 0) {
    PClip  clip = m_res.AsClip();
    for (int i = 0; i < m_frameCount; ++i) {
      clip->GetFrame(i, m_env);
    }
  }
  this->closing();
}
