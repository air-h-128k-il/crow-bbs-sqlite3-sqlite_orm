#include <memory>
#include <stdexcept>
#include "crow_all.h"
#include <sqlite_orm/sqlite_orm.h>

struct Post {
    int id;
    std::string text;
    std::string created;
};

int
main() {
  auto storage = sqlite_orm::make_storage("bbs.db",
      sqlite_orm::make_table("bbs",
        sqlite_orm::make_column("id", &Post::id, sqlite_orm::autoincrement(), sqlite_orm::primary_key()),
        sqlite_orm::make_column("text", &Post::text),
        sqlite_orm::make_column("created", &Post::created)
  ));
  storage.sync_schema();
  crow::SimpleApp app;
  crow::mustache::set_base(".");

  CROW_ROUTE(app, "/")
  ([&] {
    crow::mustache::context ctx;
    auto posts = storage.get_all<Post>();
    int n = 0;
    for(auto &post : posts) {
      ctx["posts"][n]["id"] = post.id;
      ctx["posts"][n]["text"] = post.text;
      ctx["posts"][n]["created"] = post.created;
      n++;
    }
    return crow::mustache::load("bbs.html").render(ctx);
  });

  CROW_ROUTE(app, "/post").methods("POST"_method)
  ([&](const crow::request& req, crow::response& res) {
    crow::query_string params(std::string("?") + req.body);
    char* q = params.get("text");
    if (q == nullptr) {
      res = crow::response(400);
      res.write("bad request");
      res.end();
      return;
    }

    Post post = {.text = q, .created = storage.select(sqlite_orm::datetime("now", "localtime")).front()};
    storage.insert(post);
    res = crow::response(302);
    res.set_header("Location", "/");
    res.end();
  });

  app.port(40081)
    //.multithreaded()
    .run();
}
