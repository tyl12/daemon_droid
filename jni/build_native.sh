

export GRADLE_HOME=/usr/local/gradle-4.4
export PATH=$GRADLE_HOME/bin:$PATH

# android
export ANDROID_NDK=$HOME/Android_frombuildserver/Android/Sdk/ndk-bundle
#export ANDROID_NDK=$HOME/Android/Sdk/ndk-bundle
export ADB_HOME=$HOME/android-sdk/platform-tools/

export PATH=$ANDROID_NDK:$ANDROID_NDK/toolchains/x86_64-4.9/prebuilt/linux-x86_64/bin:/usr/local/gradle/bin:$ADB_HOME:$PATH

ndk-build

