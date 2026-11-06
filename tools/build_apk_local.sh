#!/bin/sh
# Compila la APK sin Gradle (para entornos sin acceso a Maven Central).
# Uso: SDK=/ruta/al/sdk SDL=/ruta/a/SDL tools/build_apk_local.sh [versionCode] [versionName]
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
VC=${1:-1}; VN=${2:-0.1.0}
BT=$SDK/build-tools/35.0.0; JAR=$SDK/platforms/android-34/android.jar
NDK=$(ls -d "$SDK"/ndk/* | tail -1)
A=$ROOT/android/app; O=${OUT:-$ROOT/build-apk}
mkdir -p "$O/classes" "$O/dex" "$O/stage/lib/arm64-v8a"
[ -e "$A/jni/SDL" ] || ln -s "$SDL" "$A/jni/SDL"
cmake -S "$A/jni" -B "$O/native" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 -DANDROID_STL=c++_static >/dev/null
cmake --build "$O/native" >/dev/null
cp "$O/native/libmain.so" "$O/native/SDL/libSDL2.so" "$O/stage/lib/arm64-v8a/" 2>/dev/null || cp $(find "$O/native" -name 'libmain.so' -o -name 'libSDL2.so') "$O/stage/lib/arm64-v8a/"
javac -nowarn --release 11 -classpath "$JAR" -d "$O/classes" $(find "$A/src/main/java" -name '*.java') 2>&1 | grep -v "^Note\|^warning\|Picked up" || true
"$BT/d8" --release --min-api 21 --lib "$JAR" --output "$O/dex" $(find "$O/classes" -name '*.class')
"$BT/aapt2" compile --dir "$A/src/main/res" -o "$O/res.zip"
sed 's/android:installLocation/package="com.juanmote.outfoxies"\n    android:installLocation/' "$A/src/main/AndroidManifest.xml" > "$O/AndroidManifest.xml"
"$BT/aapt2" link -o "$O/base.apk" -I "$JAR" --manifest "$O/AndroidManifest.xml" "$O/res.zip" \
  --min-sdk-version 21 --target-sdk-version 34 --version-code "$VC" --version-name "$VN"
cp "$O/dex/classes.dex" "$O/stage/"
python3 -I - "$O/base.apk" "$O/stage" <<'P'
import sys,zipfile,os
apk,st=sys.argv[1:3]
z=zipfile.ZipFile(apk,'a')
z.write(os.path.join(st,'classes.dex'),'classes.dex',zipfile.ZIP_DEFLATED)
for f in os.listdir(os.path.join(st,'lib/arm64-v8a')):
    z.write(os.path.join(st,'lib/arm64-v8a',f),'lib/arm64-v8a/'+f,zipfile.ZIP_STORED)
z.close()
P
"$BT/zipalign" -f -P 16 4 "$O/base.apk" "$O/aligned.apk"
"$BT/apksigner" sign --ks "$ROOT/android/keystore/outfoxies-release.jks" --ks-pass pass:outfoxies --key-pass pass:outfoxies \
  --out "$O/TheOutfoxies-$VN.apk" "$O/aligned.apk"
"$BT/apksigner" verify "$O/TheOutfoxies-$VN.apk"
ls -la "$O/TheOutfoxies-$VN.apk"
