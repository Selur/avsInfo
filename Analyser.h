/*
 * Analyser.h
 *
 *  Created on: 13.10.2012
 *      Author: Selur
 */

#ifndef ANALYSER_H_
#define ANALYSER_H_

#include "avisynth.h"

#include <QObject>
#include <QString>
#include <QLibrary>

class Analyser : public QObject
{
  Q_OBJECT
  public:
    Analyser(QObject *parent, const QString &input, bool walk);
    virtual ~Analyser();
    void analyse();

  private:
    QString m_currentInput;
    bool m_walk;
    AVSValue m_res;
    IScriptEnvironment* m_env;
    QLibrary m_avsDLL;
    const VideoInfo* m_inf;
    int m_frameCount;

    bool initEnv();
    bool setRessource();
    void closing();
    bool setVideoInfo();
    void showVideoInfo();
    QString getColor();
    bool loadAvisynthDLL();

  signals:
    void closeApplication();


};

#endif /* ANALYSER_H_ */
