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
