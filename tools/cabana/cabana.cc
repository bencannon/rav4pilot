#include <QApplication>
#include <QCommandLineParser>

#include "common/prefix.h"
#include "selfdrive/ui/qt/util.h"
#include "tools/cabana/mainwin.h"
#include "tools/cabana/route.h"
#include "tools/cabana/streams/devicestream.h"
#include "tools/cabana/streams/pandastream.h"
#include "tools/cabana/streams/replaystream.h"

int main(int argc, char *argv[]) {
  QCoreApplication::setApplicationName("Cabana");
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  initApp(argc, argv);
  QApplication app(argc, argv);
  app.setApplicationDisplayName("Cabana");
  app.setWindowIcon(QIcon(":cabana-icon.png"));
  utils::setTheme(settings.theme);

  QCommandLineParser cmd_parser;
  cmd_parser.addHelpOption();
  cmd_parser.addPositionalArgument("route", "the drive to replay. find your drives at connect.comma.ai");
  cmd_parser.addOption({"demo", "use a demo route instead of providing your own"});
  cmd_parser.addOption({"qcam", "load qcamera"});
  cmd_parser.addOption({"ecam", "load wide road camera"});
  cmd_parser.addOption({"stream", "read can messages from live streaming"});
  cmd_parser.addOption({"panda", "read can messages from panda"});
  cmd_parser.addOption({"panda-serial", "read can messages from panda with given serial", "panda-serial"});
  cmd_parser.addOption({"zmq", "the ip address on which to receive zmq messages", "zmq"});
  cmd_parser.addOption({"data_dir", "local directory with routes", "data_dir"});
  cmd_parser.addOption({"no-vipc", "do not output video"});
  cmd_parser.addOption({"dbc", "dbc file to open", "dbc"});
  cmd_parser.process(app);

  std::unique_ptr<OpenpilotPrefix> op_prefix;
  std::unique_ptr<AbstractStream> stream;

  if (cmd_parser.isSet("stream")) {
    stream.reset(new DeviceStream(&app, cmd_parser.value("zmq")));
  } else if (cmd_parser.isSet("panda") || cmd_parser.isSet("panda-serial")) {
    PandaStreamConfig config = {};
    if (cmd_parser.isSet("panda-serial")) {
      config.serial = cmd_parser.value("panda-serial");
    }
    stream.reset(new PandaStream(&app, config));
  } else {
    // TODO: Remove when OpenpilotPrefix supports ZMQ
#ifndef __APPLE__
    op_prefix.reset(new OpenpilotPrefix());
#endif
    uint32_t replay_flags = REPLAY_FLAG_NONE;
    if (cmd_parser.isSet("ecam")) {
      replay_flags |= REPLAY_FLAG_ECAM;
    } else if (cmd_parser.isSet("qcam")) {
      replay_flags |= REPLAY_FLAG_QCAMERA;
    } else if (cmd_parser.isSet("no-vipc")) {
      replay_flags |= REPLAY_FLAG_NO_VIPC;
    }

    const QStringList args = cmd_parser.positionalArguments();
    QString route;
    if (args.size() > 0) {
      route = args.first();
    } else if (cmd_parser.isSet("demo")) {
      route = DEMO_ROUTE;
    }

    auto replay_stream = new ReplayStream(&app);
    stream.reset(replay_stream);
    if (route.isEmpty()) {
      if (OpenRouteDialog dlg(nullptr); !dlg.exec()) {
        return 0;
      }
    } else if (!replay_stream->loadRoute(route, cmd_parser.value("data_dir"), replay_flags)) {
      return 0;
    }
  }

  MainWindow w;

  // Load DBC
  if (cmd_parser.isSet("dbc")) {
    w.loadFile(cmd_parser.value("dbc"));
  }

  w.show();
  return app.exec();
}
