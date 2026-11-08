{ lib, name, width, height }:
let
  portrait = height > width;
  size = if portrait then 256 else 210;
  x = if portrait then (width - size) / 2 else width * 0.16;
  y = if portrait then 230 else (height - size) / 2;
  number = value: toString value;
  title = lib.replaceStrings [ "&" "<" ">" "\"" "'" ] [ "&amp;" "&lt;" "&gt;" "&quot;" "&#x27;" ] name;
in ''
  <svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="${number width}" height="${number height}" viewBox="0 0 ${number width} ${number height}">
    <defs>
      <linearGradient id="background" x2="0.8" y2="1"><stop stop-color="#183a52"/><stop offset="1" stop-color="#080e19"/></linearGradient>
      <radialGradient id="glow"><stop stop-color="#5bbde8" stop-opacity=".14"/><stop offset="1" stop-color="#5bbde8" stop-opacity="0"/></radialGradient>
    </defs>
    <path fill="url(#background)" d="M0 0H${number width}V${number height}H0Z"/>
    <ellipse cx="${number (x + size / 2)}" cy="${number (y + size / 2)}" rx="${number (size * 1.2)}" ry="${number (size * 1.2)}" fill="url(#glow)"/>
    <image x="${number x}" y="${number y}" width="${number size}" height="${number size}" xlink:href="data:image/png;base64,@ICON@"/>
    <text x="${number (if portrait then width / 2 else x + size + 60)}" y="${number (if portrait then 610 else height / 2 + 18)}" text-anchor="${if portrait then "middle" else "start"}" font-family="DejaVu Sans" font-size="44" font-weight="bold" fill="#f5f8fc">${title}</text>
  </svg>
''
