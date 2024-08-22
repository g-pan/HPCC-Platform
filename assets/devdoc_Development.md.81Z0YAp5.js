import{_ as e,c as s,o as t,V as a}from"./chunks/framework.gBlNPWt_.js";const m=JSON.parse('{"title":"Development Guide","description":"","frontmatter":{},"headers":[],"relativePath":"devdoc/Development.md","filePath":"devdoc/Development.md","lastUpdated":1724329493000}'),i={name:"devdoc/Development.md"},n=a(`<h1 id="development-guide" tabindex="-1">Development Guide <a class="header-anchor" href="#development-guide" aria-label="Permalink to &quot;Development Guide&quot;">​</a></h1><h2 id="hpcc-source" tabindex="-1">HPCC Source <a class="header-anchor" href="#hpcc-source" aria-label="Permalink to &quot;HPCC Source&quot;">​</a></h2><p>The most upto date details of building the system are found on the HPCC <a href="https://github.com/hpcc-systems/HPCC-Platform/wiki/Building-HPCC" target="_blank" rel="noreferrer">Wiki</a>.</p><h2 id="getting-the-sources" tabindex="-1">Getting the sources <a class="header-anchor" href="#getting-the-sources" aria-label="Permalink to &quot;Getting the sources&quot;">​</a></h2><p>The HPCC Platform sources are hosted on <a href="https://github.com/hpcc-systems/HPCC-Platform" target="_blank" rel="noreferrer">GitHub</a>. You can download a snapshot of any branch using the download button there, or you can set up a git clone of the repository. If you are planning to contribute changes to the system, see the <a href="https://github.com/hpcc-systems/HPCC-Platform/blob/master/CONTRIBUTORS" target="_blank" rel="noreferrer">CONTRIBUTORS</a> document for information about how to set up a GitHub fork of the project through which pull-requests can be made.</p><h2 id="building-the-system-from-sources" tabindex="-1">Building the system from sources <a class="header-anchor" href="#building-the-system-from-sources" aria-label="Permalink to &quot;Building the system from sources&quot;">​</a></h2><h3 id="requirements" tabindex="-1">Requirements <a class="header-anchor" href="#requirements" aria-label="Permalink to &quot;Requirements&quot;">​</a></h3><p>The HPCC platform requires a number of third party tools and libraries in order to build. The <a href="https://github.com/hpcc-systems/HPCC-Platform/wiki/Building-HPCC" target="_blank" rel="noreferrer">HPCC Wiki</a> contains the details of the dependencies that are required for different distributions.</p><p>For building any documentation, the following are also required:</p><div class="language-bash vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">bash</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">sudo</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;"> apt-get install docbook</span></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">sudo</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;"> apt-get install xsltproc</span></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">sudo</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;"> apt-get install fop</span></span></code></pre></div><p><strong>NOTE:</strong> Installing the above via alternative methods (i.e. from source) may place installations outside of searched paths.</p><h3 id="building-the-system" tabindex="-1">Building the system <a class="header-anchor" href="#building-the-system" aria-label="Permalink to &quot;Building the system&quot;">​</a></h3><p>The HPCC system is built using the cross-platform build tool cmake, which is available for Windows, virtually all flavors of Linux, FreeBSD, and other platforms. You should install cmake version 2.8.3 or later before building the sources.</p><p>On some distros you will need to build cmake from sources if the version of cmake in the standard repositories for that distro is not modern enough. It is good practice in cmake to separate the build directory where objects and executables are made from the source directory, and the HPCC cmake scripts will enforce this.</p><p>To build the sources, create a directory where the built files should be located, and from that directory, run:</p><div class="language-bash vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">bash</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">cmake</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;"> &lt;</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">source director</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">y</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&gt;</span></span></code></pre></div><p>Depending on your operating system and the compilers installed on it, this will create a makefile, Visual Studio .sln file, or other build script for building the system. If cmake was configured to create a makefile, then you can build simply by typing:</p><div class="language-bash vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">bash</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">make</span></span></code></pre></div><p>If a Visual Studio solution file was created, you can load it simply by typing the name: <code>hpccsystems-platform.sln</code></p><p>This will load the solution in Visual Studio where you can build in the usual way.</p><h2 id="packaging" tabindex="-1">Packaging <a class="header-anchor" href="#packaging" aria-label="Permalink to &quot;Packaging&quot;">​</a></h2><p>To make an installation package on a supported linux system, use the command:</p><div class="language-bash vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">bash</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">make</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;"> package</span></span></code></pre></div><p>This will first do a make to ensure everything is up to date, then will create the appropriate package for your operating system, Currently supported package formats are rpm (for RedHat/Centos) and .deb (for Debian and Ubuntu). If the operating system is not one of the above, or is not recognized, make package will create a tarball.</p><p>The package installation does not start the service on the machine, so if you want to give it a go or test it (see below), make sure to start the service manually and wait until all services are up (mainly wait for EclWatch to come up on port 8010).</p><h2 id="testing-the-system" tabindex="-1">Testing the system <a class="header-anchor" href="#testing-the-system" aria-label="Permalink to &quot;Testing the system&quot;">​</a></h2><p>After compiling, installing the package and starting the services, you can test the HPCC platform on a single-node setup.</p><h3 id="unit-tests" tabindex="-1">Unit Tests <a class="header-anchor" href="#unit-tests" aria-label="Permalink to &quot;Unit Tests&quot;">​</a></h3><p>Some components have their own unit-tests. Once you have compiled (no need to start the services), you can already run them. Supposing you build a Debug version, from the build directory you can run:</p><div class="language-bash vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">bash</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">./Debug/bin/roxie</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> -selftest</span></span></code></pre></div><p>and:</p><div class="language-bash vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">bash</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">./Debug/bin/eclagent</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> -selftest</span></span></code></pre></div><p>You can also run the Dali regression self-tests:</p><div class="language-bash vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">bash</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">./Debug/bin/daregress</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;"> localhost</span></span></code></pre></div><h3 id="regression-tests" tabindex="-1">Regression Tests <a class="header-anchor" href="#regression-tests" aria-label="Permalink to &quot;Regression Tests&quot;">​</a></h3><p><strong>MORE</strong> Completely out of date - needs rewriting.</p><h3 id="compiler-tests" tabindex="-1">Compiler Tests <a class="header-anchor" href="#compiler-tests" aria-label="Permalink to &quot;Compiler Tests&quot;">​</a></h3><p>The ECLCC compiler tests rely on two distinct runs: a known good one and your test build. For normal development, you can safely assume that the OSS/master branch in github is good. For overnight testing, golden directories need to be maintained according to the test infrastructure. There are Bash (Linux) and Batch (Windows) scripts to run the regressions:</p><p>The basic idea behind this tests is to compare the output files (logs and XML files) between runs. The log files should change slightly (the comparison should be good enough to filter most irrelevant differences), but the XML files should be identical if nothing has changed. You should only see differences in the XML where you have changed in the code, or new tests were added as part of your development.</p><p>On Linux, there are two steps:</p><p>Step 1: Check-out OSS/master, compile and run the regressions to populate the &#39;golden&#39; directory:</p><div class="language-bash vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">bash</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">./regress.sh</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> -t</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;"> golden -e buildDir/Debug/bin/eclcc</span></span></code></pre></div><p>This will run the regressions in parallel, using as many CPUs as you have, and using your just-compiled ECLCC, assuming you compiled for Debug version.</p><p>Step 2: Make your changes (or check-out your branch), compile and run again, this time output to a new directory and compare to the &#39;golden&#39; repo.:</p><div class="language-bash vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">bash</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">./regress.sh</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> -t</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;"> my_branch -c golden -e buildDir/Debug/bin/eclcc</span></span></code></pre></div><p>This will run the regressions in the same way, output to &#39;my_branch&#39; dir and compare it to the golden version, highlighting the differences.</p><p>NOTE: If you changed the headers that the compiled binaries will use, you must re-install the package (or provide -i option to the script to the new headers).</p><p>Step 3: Step 2 only listed the differences, now you need to see what they are. For that, re-run the regressing script omitting the compiler, since the only thing we&#39;ll do is to compare verbosely.:</p><div class="language-bash vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">bash</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">./regress.sh</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> -t</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;"> my_branch -c golden</span></span></code></pre></div><p>This will show you all differences, using the same ignore filters as before, between your two branches. Once you&#39;re happy with the differences, commit and issue a pull-request.</p><p>TODO: Describe compiler tests on Windows.</p><h2 id="debugging-the-system" tabindex="-1">Debugging the system <a class="header-anchor" href="#debugging-the-system" aria-label="Permalink to &quot;Debugging the system&quot;">​</a></h2><p>On linux systems, the makefile generated by cmake will build a specific version (debug or release) of the system depending on the options selected when cmake is first run in that directory. The default is to build a release system. In order to build a debug system instead, use command:</p><div class="language-bash vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">bash</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">cmake</span><span style="--shiki-light:#005CC5;--shiki-dark:#79B8FF;"> -DCMAKE_BUILD_TYPE=Debug</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;"> &lt;</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">source director</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">y</span><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">&gt;</span></span></code></pre></div><p>You can then run make or make package in the usual way to build the system.</p><p>On a Windows system, cmake always generates s solution file with both debug and release target platforms in it, so you can select which one to build within Visual Studio.</p>`,56),o=[n];function r(l,h,p,d,c,u){return t(),s("div",null,o)}const k=e(i,[["render",r]]);export{m as __pageData,k as default};
