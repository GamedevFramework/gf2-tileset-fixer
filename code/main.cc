// SPDX-License-Identifier: Zlib
// Copyright (c) 2026 Arthur Hugeat

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <gf2/core/Color.h>
#include <gf2/core/Image.h>
#include <gf2/core/Log.h>
#include <gf2/core/Vec2.h>

namespace {

  struct TilesetInfo {
    std::filesystem::path export_path;
    gf::Vec2I layout = {0, 0};
    std::vector<std::filesystem::path> asset_paths;
  };

  std::vector<std::filesystem::path> list_assets(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> asset_paths;
    for (const auto& asset: std::filesystem::directory_iterator(directory)) {
      auto asset_path = asset.path();
      if (asset_path.string().ends_with(".png")) {
        asset_paths.push_back(asset);
      }
    }
    std::sort(asset_paths.begin(), asset_paths.end());

    return asset_paths;
  }

  void generate_tileset(const TilesetInfo& tileset)
  {
    // List all assets
    std::vector<std::filesystem::path> asset_paths;
    for (const std::filesystem::path& tileset_path : tileset.asset_paths) {
      if (std::filesystem::is_directory(tileset_path)) {
        const auto assets = list_assets(tileset_path);
        asset_paths.insert(asset_paths.end(), assets.begin(), assets.end());
      } else {
        asset_paths.emplace_back(tileset_path);
      }
    }

    if (asset_paths.empty()) {
      gf::Log::warning("No assets found");
      return;
    }

    const gf::Vec2I OriginalTileSize = gf::Image(asset_paths.front()).size();
    const gf::Vec2I NewTileSize = OriginalTileSize + 2;
    const gf::Vec2I TilesetLayout = tileset.layout;
    assert(asset_paths.size() <= std::size_t(TilesetLayout.w * TilesetLayout.h));
    const gf::Vec2I TilesetSize = TilesetLayout * NewTileSize;

    gf::Image image(TilesetSize, gf::Transparent);
    for (std::size_t index = 0; index < asset_paths.size(); ++index) {
      gf::Image tile_image(asset_paths[index]);

      if (tile_image.size() != OriginalTileSize) {
        gf::Log::warning("Tile with a different size: {}", asset_paths[index].string());
        continue;
      }
      const gf::Vec2I TileIndex(std::int32_t(index) % TilesetLayout.w, std::int32_t(index) / TilesetLayout.w);

      // Copy the image
      for (int index_x = 0; index_x < OriginalTileSize.w; ++index_x) {
        for (int index_y = 0; index_y < OriginalTileSize.h; ++index_y) {
          image.put_pixel((TileIndex * NewTileSize) + gf::vec(index_x + 1, index_y + 1), tile_image({index_x, index_y}));
        }
      }

      // Copy the border
      for (int index_x = 0; index_x < OriginalTileSize.w; ++index_x) {
        image.put_pixel((TileIndex * NewTileSize) + gf::vec(index_x + 1, 0), tile_image({index_x, 0}));
        image.put_pixel((TileIndex * NewTileSize) + gf::vec(index_x + 1, NewTileSize.h - 1), tile_image({index_x, OriginalTileSize.h - 1}));
      }

      for (int index_y = 0; index_y < OriginalTileSize.h; ++index_y) {
        image.put_pixel((TileIndex * NewTileSize) + gf::vec(0, index_y + 1), tile_image({0, index_y}));
        image.put_pixel((TileIndex * NewTileSize) + gf::vec(NewTileSize.w - 1, index_y + 1), tile_image({OriginalTileSize.w - 1, index_y}));
      }

      // Copy corner
      image.put_pixel((TileIndex * NewTileSize) + gf::vec(0, 0), tile_image({0, 0}));
      image.put_pixel((TileIndex * NewTileSize) + gf::vec(NewTileSize.w - 1, 0), tile_image({OriginalTileSize.w - 1, 0}));
      image.put_pixel((TileIndex * NewTileSize) + gf::vec(NewTileSize.w - 1, NewTileSize.h - 1), tile_image({OriginalTileSize.w - 1, OriginalTileSize.h - 1}));
      image.put_pixel((TileIndex * NewTileSize) + gf::vec(0, NewTileSize.h - 1), tile_image({0, OriginalTileSize.h - 1}));
    }

    if (!std::filesystem::exists(tileset.export_path.parent_path())) {
      std::filesystem::create_directories(tileset.export_path.parent_path());
    }

    image.save_to_file(tileset.export_path);
  }

}

int main(int argc, char* argv[])
{
  if (argc != 2) {
    gf::Log::error("Missing parameter");
    gf::Log::info("Usage:");
    gf::Log::info("\t{} JSON_FILE", argv[0]);
    return 1;
  }

  std::filesystem::path recipe_path = argv[1];
  if (!std::filesystem::is_regular_file(recipe_path)) {
    gf::Log::fatal("Invalid recipe file");
    return 1;
  }

  nlohmann::json input_json = nlohmann::json::parse(std::ifstream(recipe_path));

  const auto& tilesets_json = input_json["tilesets"];
  for (const auto& tileset_json : tilesets_json) {
    TilesetInfo tileset;
    tileset.export_path = recipe_path.parent_path() / std::string(tileset_json["export_path"]);
    tileset.layout.w = tileset_json["layout"]["width"];
    tileset.layout.h = tileset_json["layout"]["height"];

    for (const std::string asset_path : tileset_json["asset_paths"]) {
      tileset.asset_paths.emplace_back(recipe_path.parent_path() / asset_path);
    }

    generate_tileset(tileset);
  }

  return 0;
}
