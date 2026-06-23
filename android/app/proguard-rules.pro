# Keep kotlinx.serialization generated serializers.
-keepattributes *Annotation*, InnerClasses
-dontnote kotlinx.serialization.**
-keepclassmembers class **$$serializer { *; }
-keepclasseswithmembers class com.pspv2.launcher.data.** {
    kotlinx.serialization.KSerializer serializer(...);
}
-keep,includedescriptorclasses class com.pspv2.launcher.data.**$$serializer { *; }
-keep class com.pspv2.launcher.data.** { *; }

# Apache Commons Compress / XZ / junrar: codecs are looked up reflectively, and the
# libraries reference optional formats we don't bundle. Keep what we use and silence
# missing references.
-keep class org.apache.commons.compress.** { *; }
-keep class org.tukaani.xz.** { *; }
-keep class com.github.junrar.** { *; }
-dontwarn org.apache.commons.compress.**
-dontwarn org.tukaani.xz.**
-dontwarn com.github.junrar.**
# junrar logs through slf4j-api, whose static binder is provided at runtime by an
# optional backend we don't bundle. Silence the missing-class reference.
-dontwarn org.slf4j.**

# Credential Manager + Google ID token sign-in: keep the public API surface used via
# reflection/Play services, and silence optional references.
-keep class androidx.credentials.** { *; }
-keep class com.google.android.libraries.identity.googleid.** { *; }
-dontwarn com.google.android.libraries.identity.googleid.**
