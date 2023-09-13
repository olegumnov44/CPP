  #include "json.h"
  #include "json_reader.h"
  #include "map_renderer.h"
  #include "request_handler.h"
  #include "serialization.h"
  #include "transport_catalogue.h"
  #include "transport_router.h"
  
  #include <transport_catalogue.pb.h>
  
  #include <fstream>
  #include <iostream>
  #include <string_view>
  
  using namespace std::literals;
  
  void PrintUsage(std::ostream& stream = std::cerr) {
      stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
  }
  
  const std::string& GetBaseFileName(const json::Dict& data) {
      return data.at("serialization_settings"s).AsDict().at("file"s).AsString();
  }
  
  
  int main(int argc, char* argv[]) {
      if (argc != 2) {
          PrintUsage();
          return 1;
      }
  
      const std::string_view mode(argv[1]);
  
      try {
          using namespace transport_catalogue;
  
          const auto json = json::Load(std::cin).GetRoot().AsDict();
  
          if (mode == "make_base"sv) {
  
              const auto db = json_reader::ReadTransportCatalogue(json.at("base_requests"s).AsArray());
  
              renderer::RenderSettings render_settings
                  = json_reader::ReadRenderSettings(json.at("render_settings"s).AsDict());
              const renderer::MapRenderer renderer{std::move(render_settings), db};
  
              router::RoutingSettings routing_settings
                  = json_reader::ReadRoutingSettings(json.at("routing_settings"s).AsDict());
              const router::Router router{std::move(routing_settings), db};
  
              const std::string& file_name = GetBaseFileName(json);
              std::ofstream file(file_name, std::ios::binary);
              assert(serialization::SerializeUniverse(db, renderer, router).SerializeToOstream(&file));
  
          } else if (mode == "process_requests"sv) {
  
              const std::string& file_name = GetBaseFileName(json);
              std::ifstream file(file_name, std::ios::binary);
              transport_catalogue_proto::TransportCatalogue proto;
              assert(proto.ParseFromIstream(&file));
  
              const auto [db, renderer, router] = serialization::DeserializeUniverse(std::move(proto));
  
              service::RequestHandler handler(db, renderer, router);
  
              json::Array responses
                  = json_reader::HandleRequests(json.at("stat_requests"s).AsArray(), handler);
  
              json::Print(json::Document(std::move(responses)), std::cout);
  
          } else {
  
              PrintUsage();
              return 1;
  
          }
  
      } catch (const std::exception& e) {
          std::cerr << "Error: "sv << e.what() << std::endl;
      }
  }
  
  
  
/*
--Задание 3
Завершите работу, начатую в предыдущих задачах: добавьте сериализацию маршрутизатора. Поскольку его построение имеет кубическую сложность по времени, а итоговый размер квадратичен по памяти, программа process_requests должна работать с готовым маршрутизатором.
Изменения по сравнению с предыдущей частью следующие:

    В "stat_requests" на входе программы process_requests добавляются запросы Route в том же формате, что и раньше.
    Начинает использоваться "routing_settings" из входа программы make_base.
    Ответ на запрос Route должен соответствовать прежнему формату (см. примеры).
    Сериализованный файл должен содержать транспортный граф и данные, необходимые для быстрого построения кратчайших путей в нём.

--Пример 1

    Ввод make_base

  {
      "serialization_settings": {
          "file": "transport_catalogue.db"
      },
      "routing_settings": {
          "bus_wait_time": 2,
          "bus_velocity": 30
      },
      "render_settings": {
          "width": 1200,
          "height": 500,
          "padding": 50,
          "stop_radius": 5,
          "line_width": 14,
          "bus_label_font_size": 20,
          "bus_label_offset": [
              7,
              15
          ],
          "stop_label_font_size": 18,
          "stop_label_offset": [
              7,
              -3
          ],
          "underlayer_color": [
              255,
              255,
              255,
              0.85
          ],
          "underlayer_width": 3,
          "color_palette": [
              "green",
              [
                  255,
                  160,
                  0
              ],
              "red"
          ]
      },
      "base_requests": [
          {
              "type": "Bus",
              "name": "14",
              "stops": [
                  "Улица Лизы Чайкиной",
                  "Электросети",
                  "Ривьерский мост",
                  "Гостиница Сочи",
                  "Кубанская улица",
                  "По требованию",
                  "Улица Докучаева",
                  "Улица Лизы Чайкиной"
              ],
              "is_roundtrip": true
          },
          {
              "type": "Bus",
              "name": "24",
              "stops": [
                  "Улица Докучаева",
                  "Параллельная улица",
                  "Электросети",
                  "Санаторий Родина"
              ],
              "is_roundtrip": false
          },
          {
              "type": "Bus",
              "name": "114",
              "stops": [
                  "Морской вокзал",
                  "Ривьерский мост"
              ],
              "is_roundtrip": false
          },
          {
              "type": "Stop",
              "name": "Улица Лизы Чайкиной",
              "latitude": 43.590317,
              "longitude": 39.746833,
              "road_distances": {
                  "Электросети": 4300,
                  "Улица Докучаева": 2000
              }
          },
          {
              "type": "Stop",
              "name": "Морской вокзал",
              "latitude": 43.581969,
              "longitude": 39.719848,
              "road_distances": {
                  "Ривьерский мост": 850
              }
          },
          {
              "type": "Stop",
              "name": "Электросети",
              "latitude": 43.598701,
              "longitude": 39.730623,
              "road_distances": {
                  "Санаторий Родина": 4500,
                  "Параллельная улица": 1200,
                  "Ривьерский мост": 1900
              }
          },
          {
              "type": "Stop",
              "name": "Ривьерский мост",
              "latitude": 43.587795,
              "longitude": 39.716901,
              "road_distances": {
                  "Морской вокзал": 850,
                  "Гостиница Сочи": 1740
              }
          },
          {
              "type": "Stop",
              "name": "Гостиница Сочи",
              "latitude": 43.578079,
              "longitude": 39.728068,
              "road_distances": {
                  "Кубанская улица": 320
              }
          },
          {
              "type": "Stop",
              "name": "Кубанская улица",
              "latitude": 43.578509,
              "longitude": 39.730959,
              "road_distances": {
                  "По требованию": 370
              }
          },
          {
              "type": "Stop",
              "name": "По требованию",
              "latitude": 43.579285,
              "longitude": 39.733742,
              "road_distances": {
                  "Улица Докучаева": 600
              }
          },
          {
              "type": "Stop",
              "name": "Улица Докучаева",
              "latitude": 43.585586,
              "longitude": 39.733879,
              "road_distances": {
                  "Параллельная улица": 1100
              }
          },
          {
              "type": "Stop",
              "name": "Параллельная улица",
              "latitude": 43.590041,
              "longitude": 39.732886,
              "road_distances": {}
          },
          {
              "type": "Stop",
              "name": "Санаторий Родина",
              "latitude": 43.601202,
              "longitude": 39.715498,
              "road_distances": {}
          }
      ]
  }
   

Ввод process_requests

  {
      "serialization_settings": {
          "file": "transport_catalogue.db"
      },
      "stat_requests": [
          {
              "id": 218563507,
              "type": "Bus",
              "name": "14"
          },
          {
              "id": 508658276,
              "type": "Stop",
              "name": "Электросети"
          },
          {
              "id": 1964680131,
              "type": "Route",
              "from": "Морской вокзал",
              "to": "Параллельная улица"
          },
          {
              "id": 1359372752,
              "type": "Map"
          }
      ]
  }
   

Вывод process_requests

  [
      {
          "curvature": 1.60481,
          "request_id": 218563507,
          "route_length": 11230,
          "stop_count": 8,
          "unique_stop_count": 7
      },
      {
          "buses": [
              "14",
              "24"
          ],
          "request_id": 508658276
      },
      {
          "items": [
              {
                  "stop_name": "Морской вокзал",
                  "time": 2,
                  "type": "Wait"
              },
              {
                  "bus": "114",
                  "span_count": 1,
                  "time": 1.7,
                  "type": "Bus"
              },
              {
                  "stop_name": "Ривьерский мост",
                  "time": 2,
                  "type": "Wait"
              },
              {
                  "bus": "14",
                  "span_count": 4,
                  "time": 6.06,
                  "type": "Bus"
              },
              {
                  "stop_name": "Улица Докучаева",
                  "time": 2,
                  "type": "Wait"
              },
              {
                  "bus": "24",
                  "span_count": 1,
                  "time": 2.2,
                  "type": "Bus"
              }
          ],
          "request_id": 1964680131,
          "total_time": 15.96
      },
      {
          "map": "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n  <polyline points=\"125.25,382.708 74.2702,281.925 125.25,382.708\" fill=\"none\" stroke=\"green\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n  <polyline points=\"592.058,238.297 311.644,93.2643 74.2702,281.925 267.446,450 317.457,442.562 365.599,429.138 367.969,320.138 592.058,238.297\" fill=\"none\" stroke=\"rgb(255,160,0)\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n  <polyline points=\"367.969,320.138 350.791,243.072 311.644,93.2643 50,50 311.644,93.2643 350.791,243.072 367.969,320.138\" fill=\"none\" stroke=\"red\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"125.25\" y=\"382.708\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n  <text fill=\"green\" x=\"125.25\" y=\"382.708\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"74.2702\" y=\"281.925\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n  <text fill=\"green\" x=\"74.2702\" y=\"281.925\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"592.058\" y=\"238.297\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">14</text>\n  <text fill=\"rgb(255,160,0)\" x=\"592.058\" y=\"238.297\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">14</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"367.969\" y=\"320.138\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">24</text>\n  <text fill=\"red\" x=\"367.969\" y=\"320.138\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">24</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"50\" y=\"50\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">24</text>\n  <text fill=\"red\" x=\"50\" y=\"50\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">24</text>\n  <circle cx=\"267.446\" cy=\"450\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"317.457\" cy=\"442.562\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"125.25\" cy=\"382.708\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"350.791\" cy=\"243.072\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"365.599\" cy=\"429.138\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"74.2702\" cy=\"281.925\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"50\" cy=\"50\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"367.969\" cy=\"320.138\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"592.058\" cy=\"238.297\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"311.644\" cy=\"93.2643\" r=\"5\" fill=\"white\"/>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"267.446\" y=\"450\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Гостиница Сочи</text>\n  <text fill=\"black\" x=\"267.446\" y=\"450\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Гостиница Сочи</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"317.457\" y=\"442.562\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Кубанская улица</text>\n  <text fill=\"black\" x=\"317.457\" y=\"442.562\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Кубанская улица</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"125.25\" y=\"382.708\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Морской вокзал</text>\n  <text fill=\"black\" x=\"125.25\" y=\"382.708\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Морской вокзал</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"350.791\" y=\"243.072\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Параллельная улица</text>\n  <text fill=\"black\" x=\"350.791\" y=\"243.072\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Параллельная улица</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"365.599\" y=\"429.138\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">По требованию</text>\n  <text fill=\"black\" x=\"365.599\" y=\"429.138\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">По требованию</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"74.2702\" y=\"281.925\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Ривьерский мост</text>\n  <text fill=\"black\" x=\"74.2702\" y=\"281.925\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Ривьерский мост</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"50\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Санаторий Родина</text>\n  <text fill=\"black\" x=\"50\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Санаторий Родина</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"367.969\" y=\"320.138\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Улица Докучаева</text>\n  <text fill=\"black\" x=\"367.969\" y=\"320.138\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Улица Докучаева</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"592.058\" y=\"238.297\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Улица Лизы Чайкиной</text>\n  <text fill=\"black\" x=\"592.058\" y=\"238.297\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Улица Лизы Чайкиной</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"311.644\" y=\"93.2643\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Электросети</text>\n  <text fill=\"black\" x=\"311.644\" y=\"93.2643\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Электросети</text>\n</svg>",
          "request_id": 1359372752
      }
  ]
   

Пример 2

    Ввод make_base

  {
      "serialization_settings": {
          "file": "transport_catalogue.db"
      },
      "routing_settings": {
          "bus_wait_time": 2,
          "bus_velocity": 30
      },
      "render_settings": {
          "width": 1500,
          "height": 950,
          "padding": 50,
          "stop_radius": 3,
          "line_width": 10,
          "bus_label_font_size": 18,
          "bus_label_offset": [
              7,
              15
          ],
          "stop_label_font_size": 13,
          "stop_label_offset": [
              7,
              -3
          ],
          "underlayer_color": [
              255,
              255,
              255,
              0.85
          ],
          "underlayer_width": 3,
          "color_palette": [
              "red",
              "green",
              "blue",
              "brown",
              "orange"
          ]
      },
      "base_requests": [
          {
              "type": "Bus",
              "name": "23",
              "stops": [
                  "Санаторий Салют",
                  "Санаторная улица",
                  "Пансионат Нева",
                  "Санаторий Радуга",
                  "Санаторий Родина",
                  "Спортивная",
                  "Парк Ривьера",
                  "Морской вокзал",
                  "Органный зал",
                  "Театральная",
                  "Пансионат Светлана",
                  "Цирк",
                  "Стадион",
                  "Санаторий Металлург",
                  "Улица Бытха"
              ],
              "is_roundtrip": false
          },
          {
              "type": "Bus",
              "name": "13",
              "stops": [
                  "Улица Лизы Чайкиной",
                  "Пионерская улица, 111",
                  "Садовая",
                  "Театральная"
              ],
              "is_roundtrip": false
          },
          {
              "type": "Bus",
              "name": "36",
              "stops": [
                  "Морской вокзал",
                  "Сбербанк",
                  "Автовокзал",
                  "Отель Звёздный",
                  "Магазин Быт",
                  "Хлебозавод",
                  "Кинотеатр Юбилейный",
                  "Новая Заря",
                  "Деревообр. комбинат",
                  "Целинная улица, 5",
                  "Целинная улица, 57",
                  "Целинная улица"
              ],
              "is_roundtrip": false
          },
          {
              "type": "Bus",
              "name": "44к",
              "stops": [
                  "Пансионат Светлана",
                  "Улица Лысая Гора",
                  "Улица В. Лысая Гора"
              ],
              "is_roundtrip": false
          },
          {
              "type": "Bus",
              "name": "90",
              "stops": [
                  "Краево-Греческая улица",
                  "Улица Бытха",
                  "Санаторий им. Ворошилова",
                  "Санаторий Приморье",
                  "Санаторий Заря",
                  "Мацеста",
                  "Мацестинская долина"
              ],
              "is_roundtrip": false
          },
          {
              "type": "Stop",
              "name": "Краево-Греческая улица",
              "latitude": 43.565551,
              "longitude": 39.776858,
              "road_distances": {
                  "Улица Бытха": 1780
              }
          },
          {
              "type": "Stop",
              "name": "Санаторий им. Ворошилова",
              "latitude": 43.557935,
              "longitude": 39.764452,
              "road_distances": {
                  "Санаторий Приморье": 950
              }
          },
          {
              "type": "Stop",
              "name": "Санаторий Приморье",
              "latitude": 43.554202,
              "longitude": 39.77256,
              "road_distances": {
                  "Санаторий Заря": 2350
              }
          },
          {
              "type": "Stop",
              "name": "Санаторий Заря",
              "latitude": 43.549618,
              "longitude": 39.780908,
              "road_distances": {
                  "Мацеста": 800
              }
          },
          {
              "type": "Stop",
              "name": "Мацеста",
              "latitude": 43.545509,
              "longitude": 39.788993,
              "road_distances": {
                  "Мацестинская долина": 2350
              }
          },
          {
              "type": "Stop",
              "name": "Мацестинская долина",
              "latitude": 43.560422,
              "longitude": 39.798219,
              "road_distances": {}
          },
          {
              "type": "Stop",
              "name": "Улица Лысая Гора",
              "latitude": 43.577997,
              "longitude": 39.741685,
              "road_distances": {
                  "Улица В. Лысая Гора": 640
              }
          },
          {
              "type": "Stop",
              "name": "Улица В. Лысая Гора",
              "latitude": 43.58092,
              "longitude": 39.744749,
              "road_distances": {}
          },
          {
              "type": "Stop",
              "name": "Морской вокзал",
              "latitude": 43.581969,
              "longitude": 39.719848,
              "road_distances": {
                  "Сбербанк": 870,
                  "Органный зал": 570
              }
          },
          {
              "type": "Stop",
              "name": "Сбербанк",
              "latitude": 43.585969,
              "longitude": 39.725175,
              "road_distances": {
                  "Автовокзал": 870
              }
          },
          {
              "type": "Stop",
              "name": "Автовокзал",
              "latitude": 43.592956,
              "longitude": 39.727798,
              "road_distances": {
                  "Отель Звёздный": 700
              }
          },
          {
              "type": "Stop",
              "name": "Отель Звёздный",
              "latitude": 43.596585,
              "longitude": 39.721151,
              "road_distances": {
                  "Магазин Быт": 1000
              }
          },
          {
              "type": "Stop",
              "name": "Магазин Быт",
              "latitude": 43.604025,
              "longitude": 39.724492,
              "road_distances": {
                  "Хлебозавод": 420
              }
          },
          {
              "type": "Stop",
              "name": "Хлебозавод",
              "latitude": 43.607364,
              "longitude": 39.726643,
              "road_distances": {
                  "Кинотеатр Юбилейный": 2110
              }
          },
          {
              "type": "Stop",
              "name": "Кинотеатр Юбилейный",
              "latitude": 43.623382,
              "longitude": 39.720626,
              "road_distances": {
                  "Новая Заря": 450
              }
          },
          {
              "type": "Stop",
              "name": "Новая Заря",
              "latitude": 43.626842,
              "longitude": 39.717802,
              "road_distances": {
                  "Деревообр. комбинат": 530
              }
          },
          {
              "type": "Stop",
              "name": "Деревообр. комбинат",
              "latitude": 43.631035,
              "longitude": 39.714624,
              "road_distances": {
                  "Целинная улица, 5": 840
              }
          },
          {
              "type": "Stop",
              "name": "Целинная улица, 5",
              "latitude": 43.633353,
              "longitude": 39.710257,
              "road_distances": {
                  "Целинная улица, 57": 1270
              }
          },
          {
              "type": "Stop",
              "name": "Целинная улица, 57",
              "latitude": 43.640536,
              "longitude": 39.713253,
              "road_distances": {
                  "Целинная улица": 1050
              }
          },
          {
              "type": "Stop",
              "name": "Целинная улица",
              "latitude": 43.647968,
              "longitude": 39.717733,
              "road_distances": {}
          },
          {
              "type": "Stop",
              "name": "Санаторий Салют",
              "latitude": 43.623238,
              "longitude": 39.704646,
              "road_distances": {
                  "Санаторная улица": 1500
              }
          },
          {
              "type": "Stop",
              "name": "Санаторная улица",
              "latitude": 43.620766,
              "longitude": 39.719058,
              "road_distances": {
                  "Пансионат Нева": 670
              }
          },
          {
              "type": "Stop",
              "name": "Пансионат Нева",
              "latitude": 43.614288,
              "longitude": 39.718674,
              "road_distances": {
                  "Санаторий Радуга": 520
              }
          },
          {
              "type": "Stop",
              "name": "Санаторий Радуга",
              "latitude": 43.609951,
              "longitude": 39.72143,
              "road_distances": {
                  "Санаторий Родина": 1190
              }
          },
          {
              "type": "Stop",
              "name": "Санаторий Родина",
              "latitude": 43.601202,
              "longitude": 39.715498,
              "road_distances": {
                  "Спортивная": 1100
              }
          },
          {
              "type": "Stop",
              "name": "Спортивная",
              "latitude": 43.593689,
              "longitude": 39.717642,
              "road_distances": {
                  "Парк Ривьера": 640
              }
          },
          {
              "type": "Stop",
              "name": "Парк Ривьера",
              "latitude": 43.588296,
              "longitude": 39.715956,
              "road_distances": {
                  "Морской вокзал": 730
              }
          },
          {
              "type": "Stop",
              "name": "Органный зал",
              "latitude": 43.57926,
              "longitude": 39.725574,
              "road_distances": {
                  "Театральная": 770
              }
          },
          {
              "type": "Stop",
              "name": "Пансионат Светлана",
              "latitude": 43.571807,
              "longitude": 39.735866,
              "road_distances": {
                  "Цирк": 520,
                  "Улица Лысая Гора": 1070
              }
          },
          {
              "type": "Stop",
              "name": "Цирк",
              "latitude": 43.569207,
              "longitude": 39.739869,
              "road_distances": {
                  "Стадион": 860
              }
          },
          {
              "type": "Stop",
              "name": "Стадион",
              "latitude": 43.565301,
              "longitude": 39.749485,
              "road_distances": {
                  "Санаторий Металлург": 950
              }
          },
          {
              "type": "Stop",
              "name": "Санаторий Металлург",
              "latitude": 43.561005,
              "longitude": 39.760511,
              "road_distances": {
                  "Улица Бытха": 900
              }
          },
          {
              "type": "Stop",
              "name": "Улица Бытха",
              "latitude": 43.566135,
              "longitude": 39.762109,
              "road_distances": {
                  "Санаторий им. Ворошилова": 1160
              }
          },
          {
              "type": "Stop",
              "name": "Улица Лизы Чайкиной",
              "latitude": 43.590317,
              "longitude": 39.746833,
              "road_distances": {
                  "Пионерская улица, 111": 950
              }
          },
          {
              "type": "Stop",
              "name": "Пионерская улица, 111",
              "latitude": 43.587257,
              "longitude": 39.740325,
              "road_distances": {
                  "Садовая": 520
              }
          },
          {
              "type": "Stop",
              "name": "Садовая",
              "latitude": 43.58395,
              "longitude": 39.736938,
              "road_distances": {
                  "Театральная": 1300
              }
          },
          {
              "type": "Stop",
              "name": "Театральная",
              "latitude": 43.57471,
              "longitude": 39.731954,
              "road_distances": {
                  "Пансионат Светлана": 390
              }
          }
      ]
  }
   

Ввод process_requests

  {
      "serialization_settings": {
          "file": "transport_catalogue.db"
      },
      "stat_requests": [
          {
              "id": 80146768,
              "type": "Bus",
              "name": "13"
          },
          {
              "id": 1571442892,
              "type": "Stop",
              "name": "Морской вокзал"
          },
          {
              "id": 1490699931,
              "type": "Route",
              "from": "Пансионат Нева",
              "to": "Улица Лысая Гора"
          },
          {
              "id": 684758285,
              "type": "Map"
          }
      ]
  }
   

Вывод process_requests

  [
      {
          "curvature": 1.26723,
          "request_id": 80146768,
          "route_length": 5540,
          "stop_count": 7,
          "unique_stop_count": 4
      },
      {
          "buses": [
              "23",
              "36"
          ],
          "request_id": 1571442892
      },
      {
          "items": [
              {
                  "stop_name": "Пансионат Нева",
                  "time": 2,
                  "type": "Wait"
              },
              {
                  "bus": "23",
                  "span_count": 8,
                  "time": 11.82,
                  "type": "Bus"
              },
              {
                  "stop_name": "Пансионат Светлана",
                  "time": 2,
                  "type": "Wait"
              },
              {
                  "bus": "44к",
                  "span_count": 1,
                  "time": 2.14,
                  "type": "Bus"
              }
          ],
          "request_id": 1490699931,
          "total_time": 17.96
      },
      {
          "map": "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n  <polyline points=\"399.983,528.273 345.993,553.659 317.894,581.093 276.547,657.748 317.894,581.093 345.993,553.659 399.983,528.273\" fill=\"none\" stroke=\"red\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n  <polyline points=\"50,255.16 169.562,275.668 166.376,329.409 189.24,365.389 140.028,437.971 157.815,500.299 143.828,545.039 176.116,597.528 223.619,620.002 276.547,657.748 309.001,681.832 342.21,703.401 421.984,735.806 513.456,771.445 526.713,728.887 513.456,771.445 421.984,735.806 342.21,703.401 309.001,681.832 276.547,657.748 223.619,620.002 176.116,597.528 143.828,545.039 157.815,500.299 140.028,437.971 189.24,365.389 166.376,329.409 169.562,275.668 50,255.16\" fill=\"none\" stroke=\"green\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n  <polyline points=\"176.116,597.528 220.309,564.344 242.069,506.38 186.926,476.273 214.642,414.551 232.487,386.851 182.57,253.965 159.142,225.261 132.778,190.476 96.5489,171.246 121.404,111.656 158.57,50 121.404,111.656 96.5489,171.246 132.778,190.476 159.142,225.261 182.57,253.965 232.487,386.851 214.642,414.551 186.926,476.273 242.069,506.38 220.309,564.344 176.116,597.528\" fill=\"none\" stroke=\"blue\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n  <polyline points=\"309.001,681.832 357.276,630.48 382.695,606.23 357.276,630.48 309.001,681.832\" fill=\"none\" stroke=\"brown\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n  <polyline points=\"649.071,733.732 526.713,728.887 546.151,796.914 613.415,827.883 682.67,865.912 749.743,900 826.282,776.282 749.743,900 682.67,865.912 613.415,827.883 546.151,796.914 526.713,728.887 649.071,733.732\" fill=\"none\" stroke=\"orange\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"399.983\" y=\"528.273\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">13</text>\n  <text fill=\"red\" x=\"399.983\" y=\"528.273\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">13</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"276.547\" y=\"657.748\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">13</text>\n  <text fill=\"red\" x=\"276.547\" y=\"657.748\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">13</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"50\" y=\"255.16\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">23</text>\n  <text fill=\"green\" x=\"50\" y=\"255.16\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">23</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"526.713\" y=\"728.887\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">23</text>\n  <text fill=\"green\" x=\"526.713\" y=\"728.887\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">23</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"176.116\" y=\"597.528\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">36</text>\n  <text fill=\"blue\" x=\"176.116\" y=\"597.528\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">36</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"158.57\" y=\"50\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">36</text>\n  <text fill=\"blue\" x=\"158.57\" y=\"50\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">36</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"309.001\" y=\"681.832\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">44к</text>\n  <text fill=\"brown\" x=\"309.001\" y=\"681.832\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">44к</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"382.695\" y=\"606.23\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">44к</text>\n  <text fill=\"brown\" x=\"382.695\" y=\"606.23\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">44к</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"649.071\" y=\"733.732\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">90</text>\n  <text fill=\"orange\" x=\"649.071\" y=\"733.732\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">90</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"826.282\" y=\"776.282\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">90</text>\n  <text fill=\"orange\" x=\"826.282\" y=\"776.282\" dx=\"7\" dy=\"15\" font-size=\"18\" font-family=\"Verdana\" font-weight=\"bold\">90</text>\n  <circle cx=\"242.069\" cy=\"506.38\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"132.778\" cy=\"190.476\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"182.57\" cy=\"253.965\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"649.071\" cy=\"733.732\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"214.642\" cy=\"414.551\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"749.743\" cy=\"900\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"826.282\" cy=\"776.282\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"176.116\" cy=\"597.528\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"159.142\" cy=\"225.261\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"223.619\" cy=\"620.002\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"186.926\" cy=\"476.273\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"166.376\" cy=\"329.409\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"309.001\" cy=\"681.832\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"143.828\" cy=\"545.039\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"345.993\" cy=\"553.659\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"317.894\" cy=\"581.093\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"682.67\" cy=\"865.912\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"513.456\" cy=\"771.445\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"613.415\" cy=\"827.883\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"189.24\" cy=\"365.389\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"140.028\" cy=\"437.971\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"50\" cy=\"255.16\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"546.151\" cy=\"796.914\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"169.562\" cy=\"275.668\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"220.309\" cy=\"564.344\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"157.815\" cy=\"500.299\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"421.984\" cy=\"735.806\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"276.547\" cy=\"657.748\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"526.713\" cy=\"728.887\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"382.695\" cy=\"606.23\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"399.983\" cy=\"528.273\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"357.276\" cy=\"630.48\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"232.487\" cy=\"386.851\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"158.57\" cy=\"50\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"96.5489\" cy=\"171.246\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"121.404\" cy=\"111.656\" r=\"3\" fill=\"white\"/>\n  <circle cx=\"342.21\" cy=\"703.401\" r=\"3\" fill=\"white\"/>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"242.069\" y=\"506.38\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Автовокзал</text>\n  <text fill=\"black\" x=\"242.069\" y=\"506.38\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Автовокзал</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"132.778\" y=\"190.476\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Деревообр. комбинат</text>\n  <text fill=\"black\" x=\"132.778\" y=\"190.476\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Деревообр. комбинат</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"182.57\" y=\"253.965\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Кинотеатр Юбилейный</text>\n  <text fill=\"black\" x=\"182.57\" y=\"253.965\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Кинотеатр Юбилейный</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"649.071\" y=\"733.732\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Краево-Греческая улица</text>\n  <text fill=\"black\" x=\"649.071\" y=\"733.732\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Краево-Греческая улица</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"214.642\" y=\"414.551\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Магазин Быт</text>\n  <text fill=\"black\" x=\"214.642\" y=\"414.551\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Магазин Быт</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"749.743\" y=\"900\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Мацеста</text>\n  <text fill=\"black\" x=\"749.743\" y=\"900\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Мацеста</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"826.282\" y=\"776.282\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Мацестинская долина</text>\n  <text fill=\"black\" x=\"826.282\" y=\"776.282\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Мацестинская долина</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"176.116\" y=\"597.528\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Морской вокзал</text>\n  <text fill=\"black\" x=\"176.116\" y=\"597.528\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Морской вокзал</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"159.142\" y=\"225.261\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Новая Заря</text>\n  <text fill=\"black\" x=\"159.142\" y=\"225.261\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Новая Заря</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"223.619\" y=\"620.002\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Органный зал</text>\n  <text fill=\"black\" x=\"223.619\" y=\"620.002\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Органный зал</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"186.926\" y=\"476.273\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Отель Звёздный</text>\n  <text fill=\"black\" x=\"186.926\" y=\"476.273\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Отель Звёздный</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"166.376\" y=\"329.409\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Пансионат Нева</text>\n  <text fill=\"black\" x=\"166.376\" y=\"329.409\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Пансионат Нева</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"309.001\" y=\"681.832\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Пансионат Светлана</text>\n  <text fill=\"black\" x=\"309.001\" y=\"681.832\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Пансионат Светлана</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"143.828\" y=\"545.039\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Парк Ривьера</text>\n  <text fill=\"black\" x=\"143.828\" y=\"545.039\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Парк Ривьера</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"345.993\" y=\"553.659\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Пионерская улица, 111</text>\n  <text fill=\"black\" x=\"345.993\" y=\"553.659\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Пионерская улица, 111</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"317.894\" y=\"581.093\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Садовая</text>\n  <text fill=\"black\" x=\"317.894\" y=\"581.093\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Садовая</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"682.67\" y=\"865.912\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий Заря</text>\n  <text fill=\"black\" x=\"682.67\" y=\"865.912\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий Заря</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"513.456\" y=\"771.445\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий Металлург</text>\n  <text fill=\"black\" x=\"513.456\" y=\"771.445\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий Металлург</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"613.415\" y=\"827.883\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий Приморье</text>\n  <text fill=\"black\" x=\"613.415\" y=\"827.883\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий Приморье</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"189.24\" y=\"365.389\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий Радуга</text>\n  <text fill=\"black\" x=\"189.24\" y=\"365.389\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий Радуга</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"140.028\" y=\"437.971\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий Родина</text>\n  <text fill=\"black\" x=\"140.028\" y=\"437.971\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий Родина</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"50\" y=\"255.16\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий Салют</text>\n  <text fill=\"black\" x=\"50\" y=\"255.16\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий Салют</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"546.151\" y=\"796.914\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий им. Ворошилова</text>\n  <text fill=\"black\" x=\"546.151\" y=\"796.914\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторий им. Ворошилова</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"169.562\" y=\"275.668\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторная улица</text>\n  <text fill=\"black\" x=\"169.562\" y=\"275.668\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Санаторная улица</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"220.309\" y=\"564.344\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Сбербанк</text>\n  <text fill=\"black\" x=\"220.309\" y=\"564.344\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Сбербанк</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"157.815\" y=\"500.299\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Спортивная</text>\n  <text fill=\"black\" x=\"157.815\" y=\"500.299\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Спортивная</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"421.984\" y=\"735.806\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Стадион</text>\n  <text fill=\"black\" x=\"421.984\" y=\"735.806\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Стадион</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"276.547\" y=\"657.748\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Театральная</text>\n  <text fill=\"black\" x=\"276.547\" y=\"657.748\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Театральная</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"526.713\" y=\"728.887\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Улица Бытха</text>\n  <text fill=\"black\" x=\"526.713\" y=\"728.887\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Улица Бытха</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"382.695\" y=\"606.23\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Улица В. Лысая Гора</text>\n  <text fill=\"black\" x=\"382.695\" y=\"606.23\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Улица В. Лысая Гора</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"399.983\" y=\"528.273\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Улица Лизы Чайкиной</text>\n  <text fill=\"black\" x=\"399.983\" y=\"528.273\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Улица Лизы Чайкиной</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"357.276\" y=\"630.48\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Улица Лысая Гора</text>\n  <text fill=\"black\" x=\"357.276\" y=\"630.48\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Улица Лысая Гора</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"232.487\" y=\"386.851\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Хлебозавод</text>\n  <text fill=\"black\" x=\"232.487\" y=\"386.851\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Хлебозавод</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"158.57\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Целинная улица</text>\n  <text fill=\"black\" x=\"158.57\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Целинная улица</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"96.5489\" y=\"171.246\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Целинная улица, 5</text>\n  <text fill=\"black\" x=\"96.5489\" y=\"171.246\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Целинная улица, 5</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"121.404\" y=\"111.656\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Целинная улица, 57</text>\n  <text fill=\"black\" x=\"121.404\" y=\"111.656\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Целинная улица, 57</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"342.21\" y=\"703.401\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Цирк</text>\n  <text fill=\"black\" x=\"342.21\" y=\"703.401\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\">Цирк</text>\n</svg>",
          "request_id": 684758285
      }
  ]
   

В файле ниже - примеры случайно сгенерированных входных файлов и корректных выводов для них.
s14_part3_opentest.zip

--Подсказка
Сериализуйте настройки маршрутизации, содержимое классов Graph и Router, а также координаты остановок. В качестве аналога std::variant в proto-сообщениях используйте oneof.
Будьте внимательны при десериализации Router, содержащего ссылку на Graph. Убедитесь, что граф, на который ссылается маршрутизатор, не меняет свой адрес в памяти.

*/
