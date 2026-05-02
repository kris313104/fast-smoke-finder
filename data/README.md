# Adding Lineups

## File structure

One JSON file per map in `data/lineups/`. Filename = map id (must match the id in `src/controllers/LineupController.cc`).

## Lineup schema

```json
{
  "id":            "dust2_a_short_smoke_001",   // unique, format: {map}_{area}_{type}_{seq}
  "map":           "dust2",
  "type":          "smoke",                     // smoke | molotov | flash
  "title":         "A Short to CT Smoke",
  "description":   "Stand at T-spawn corner…",  // optional
  "tags":          ["ct", "a", "short", "doors", "entry"],
  "stance":        "standing",                  // standing | crouching | jumping
  "throw_type":    "left-click",                // left-click | right-click | jump-throw
  "position_img":  "/img/lineups/dust2_a_short_smoke_001_pos.jpg",
  "crosshair_img": "/img/lineups/dust2_a_short_smoke_001_cross.jpg",
  "video_url":     ""                           // optional YouTube/Streamable link
}
```

## Tag vocabulary

Use these to stay consistent. Mix freely — they are OR-matched.

| Category  | Tags |
|-----------|------|
| Sites     | `a`, `b`, `mid` |
| Areas     | `ct`, `short`, `long`, `ramp`, `window`, `pit`, `doors`, `balcony`, `palace`, `jungle`, `banana`, `bridge`, `van`, `hut`, `silo`, `elevator`, `stairs`, `tunnel`, `water` |
| Side      | `t-side`, `ct-side` |
| Role      | `entry`, `retake`, `post-plant`, `default`, `anti-rush`, `pop-flash` |
| Mechanics | `one-way`, `moving` |

## Adding screenshots

1. Take two screenshots in CS2 (Windows key or Steam overlay).
2. Name them `{id}_pos.jpg` and `{id}_cross.jpg`.
3. Compress to ≤150 KB at JPEG quality 80.
4. Place in `static/img/lineups/`.

## After adding

Restart the server — `LineupStore` reloads all JSON files on startup.
