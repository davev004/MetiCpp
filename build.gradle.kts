plugins {
    application
    id("org.graalvm.buildtools.native") version "0.10.4"
}

group = "engine"
version = "1.0-SNAPSHOT"

repositories {
    mavenCentral()
}

java {
    toolchain {
        languageVersion.set(JavaLanguageVersion.of(21))
    }
}

application {
    mainClass.set("engine.Main")
}

val graalLauncher = javaToolchains.launcherFor {
    languageVersion.set(JavaLanguageVersion.of(21))
}

graalvmNative {
    toolchainDetection.set(false)
    binaries {
        named("main") {
            imageName.set("janus")
            mainClass.set("engine.Main")
            javaLauncher.set(graalLauncher)
            buildArgs.addAll(
                "--no-fallback",
                "--initialize-at-build-time",
                "-O3",
                "-march=native"
            )
        }
    }
}

dependencies {
    testImplementation(platform("org.junit:junit-bom:5.11.0"))
    testImplementation("org.junit.jupiter:junit-jupiter")
    testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

tasks.test {
    useJUnitPlatform()
}
