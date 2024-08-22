import{_ as e,c as t,o as i,V as a}from"./chunks/framework.gBlNPWt_.js";const g=JSON.parse('{"title":"Current versions","description":"","frontmatter":{},"headers":[],"relativePath":"devdoc/VersionSupport.md","filePath":"devdoc/VersionSupport.md","lastUpdated":1724329493000}'),s={name:"devdoc/VersionSupport.md"},o=a('<h1 id="current-versions" tabindex="-1">Current versions <a class="header-anchor" href="#current-versions" aria-label="Permalink to &quot;Current versions&quot;">​</a></h1><table><thead><tr><th>name</th><th>version</th></tr></thead><tbody><tr><td>current</td><td>9.4.x</td></tr><tr><td>previous</td><td>9.2.x</td></tr><tr><td>critical</td><td>9.0.x</td></tr><tr><td>security</td><td>8.12.x</td></tr></tbody></table><h2 id="supported-versions" tabindex="-1">Supported versions <a class="header-anchor" href="#supported-versions" aria-label="Permalink to &quot;Supported versions&quot;">​</a></h2><p>We release a new version of the platform every 3 months. If there are major changes in functionality, or significant backward compatibility issues then it will be tagged as a new major version, otherwise a new minor version. We normally maintain 4 versions of the system, which means that each new release will typically be supported for a year. Once a new major or minor version has been tagged gold it should not have any changes that change the behavior of queries.</p><p>Which versions should changes be applied to? The following gives some examples of the types of changes and which version they would be most relevant to target.</p><p>&quot;master&quot;:</p><ul><li>New features.</li><li>Bug fixes that will change the semantics of existing queries or processes.</li><li>Refactoring.</li><li>Performance improvements (unless simple and safe)</li></ul><p>&quot;&lt;current&gt;&quot;:</p><ul><li>Bug fixes that only change behavior where it previously crashes or had undefined behavior (If well defined but wrong need to have very strong justification to change.)</li><li>Fixes for race conditions (the behavior was previously indeterminate so less of an argument against it changing)</li><li>Data corruption fixes - on a case by case basis if they change existing query results.</li><li>Missing functionality that prevents features from working.</li><li>Changes for tech-preview work that only effect those who are using it.</li><li>Regressions.</li><li>Improvements to logging and error messages (possibly in &quot;previous&quot; if simple and added to help diagnose problems).</li><li>Occasional simple refactoring that makes up-merging simpler..</li><li>Changes to improve backward compatibility of new features. (E.g. adding an ignored syntax to the compiler.)</li><li>Performance improvements - if simple and safe</li></ul><p>&quot;&lt;previous&gt;&quot;:</p><ul><li>Simple bug fixes that do not change behavior</li><li>Simple changes for missing functionality</li><li>Regressions with simple fixes (but care is needed if it caused a change in behavior)</li><li>Serious regressions</li><li>Complex security fixes</li></ul><p>&quot;&lt;critical&gt;&quot; fixes only:</p><ul><li>Simple security fixes</li><li>Complex security fixes if sufficiently serious</li></ul><p>&quot;&lt;security&gt;&quot; fixes only:</p><ul><li>Serious security fixes</li></ul><p>Occasionally earlier branches will be chosen, (e.g. security fixes to even older versions) but they should always be carefully discussed (and documented).</p><h2 id="patches-and-images" tabindex="-1">Patches and images <a class="header-anchor" href="#patches-and-images" aria-label="Permalink to &quot;Patches and images&quot;">​</a></h2><p>We aim to produce new point releases once a week. The point releases will contain</p><p>a) Any changes to the code base for that branch. b) Any security fixes for libraries that are project dependencies. We will upgrade to the latest point release for the library that fixes the security issue. c) For the cloud any security fixes in the base image or the packages installed in that image.</p><p>If there are no changes in any of those areas for a particular version then a new point release will not be created.</p><p>If you are deploying a system to the cloud you have one of two options</p><p>a) Use the images that are automatically built and published as part of the build pipeline. This image is currently based on ubuntu 22.04 and contains the majority of packages users will require.</p><p>b) Use your own hardened base image, and install the containerized package that we publish into that image.</p><h2 id="package-versions" tabindex="-1">Package versions. <a class="header-anchor" href="#package-versions" aria-label="Permalink to &quot;Package versions.&quot;">​</a></h2><p>We currently generate the following versions of the package and images:</p><ul><li>debug</li><li>release with symbols</li><li>release without symbols.</li></ul><p>It is recommended that you deploy the &quot;release with symbols&quot; version to all bare-metal and non-production cloud deployments. The extra symbols allow the system to generate stack backtraces which make it much easier to diagnose problems if they occur. The &quot;release without symbols&quot; version is recommended for Kubernetes production deployments. Deploying a system without symbols reduces the size of the images. This reduces the time it takes Kubernetes to copy the image before provisioning a new node.</p>',27),r=[o];function n(l,h,c,u,d,p){return i(),t("div",null,r)}const f=e(s,[["render",n]]);export{g as __pageData,f as default};
