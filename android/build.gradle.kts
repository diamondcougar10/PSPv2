// Top-level build file. Common configuration for all sub-projects/modules.
// AGP 9 has built-in Kotlin support, so the kotlin.android plugin is no longer
// applied; AGP brings the Kotlin Gradle plugin (2.2.10) onto the classpath, and
// the compose/serialization compiler plugins are applied without a version in
// the app module so they resolve against that bundled KGP.
plugins {
    id("com.android.application") version "9.2.1" apply false
}
