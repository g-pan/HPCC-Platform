import{_ as e,c as t,o as i,V as o}from"./chunks/framework.gBlNPWt_.js";const g=JSON.parse('{"title":"Code Submission Guidelines","description":"","frontmatter":{},"headers":[],"relativePath":"devdoc/CodeSubmissions.md","filePath":"devdoc/CodeSubmissions.md","lastUpdated":1724329493000}'),s={name:"devdoc/CodeSubmissions.md"},r=o('<h1 id="code-submission-guidelines" tabindex="-1">Code Submission Guidelines <a class="header-anchor" href="#code-submission-guidelines" aria-label="Permalink to &quot;Code Submission Guidelines&quot;">​</a></h1><p>We welcome submissions to the platform especially in the form of pull requests into the HPCC-Systems github repository. The following describes some of processes for merging PRs.</p><h2 id="pull-requests" tabindex="-1">Pull requests <a class="header-anchor" href="#pull-requests" aria-label="Permalink to &quot;Pull requests&quot;">​</a></h2><p>There are a few things that should be considered when creating a PR to increase the likelihood that they can be accepted quickly.</p><ul><li>Write a good commit message<br> The format should be HPCC-XXXXX (where XXXXX is the bug number) followed by a description of the issue. The text should make sense in a change log by itself - without reference to the jira or the contents of the PR. We should aim to increase the information that is included as part of the commit message - not rely on on the jira.</li><li>Ensure the reviewer has enough information to review the change.<br> The code reviewer only has the JIRA and the PR to go on. The JIRA (or associated documentation) should contain enough details to review the PR - e.g. the purpose, main aim, why the change was made etc.. If the scope of the jira has changed then the jira should be updated to reflect that.<br> If the submission requires changes to the documentation then the JIRA should contain all the details needed to document it, and the PR should either contain the documentation changes, or a documentation JIRA should be created.</li><li>Fill in the checklist<br> The check boxes are there to remind you to consider different aspects of the PR. Not all of them apply to every submission, but if you tick a box and have not really thought about the item then prepare to be embarrassed!</li><li>Prefer small submissions<br> It isn&#39;t always possible, but several smaller PRs are much easier to review than one large change. If your submission includes semi-automatic/mechanical changes (e.g. renaming large numbers of function calls, or adding an extra parameter) please keep it as a separate commit. This makes it much easier to review the PR - since the reviewer will be looking for different errors in the different types of changes.</li><li>Check for silly mistakes<br> Review your own code in github, after creating the PR to check for silly mistakes. It doesn&#39;t take long, and often catches trivial issues. It may avoid the need for a cycle of code-review/fixes. It may be helpful to add some notes to specific changes e.g. &quot;this change is mainly or solely refactoring method A into method B and C. &quot;. Some common examples of trivial issues to look for include: <ul><li>Inconsistent indentation, or using tabs rather than spaces to indent.</li><li>Lines of tracing left in.</li><li>Lines of code commented out that should be deleted.</li><li>TBD reminders of work that need implementing or removing.</li><li>Unrelated files that have been accidentally modified.</li><li>Accidental changes to submodule versions.</li><li>Typos in error messages, tracing or comments, or in the commit message.</li><li>Incomplete edits when copy and pasting code.</li><li>Check subtractions are the right way around, and potential for overflow.</li><li>New files with the wrong copyright date</li></ul></li><li>Check the target branch (see below)</li><li>Request one or more reviews. For relatively simple changes a single reviewer is normally enough.</li></ul><h2 id="reviewers" tabindex="-1">Reviewers <a class="header-anchor" href="#reviewers" aria-label="Permalink to &quot;Reviewers&quot;">​</a></h2><p>All pull requests should be reviewed by someone who is not the author before merging. Complex changes, changes that require input from multiple experts, or that have implications throughout the system should be reviewed by multiple reviewers. This should include someone who is responsible for merging changes for that part of the system. (Unless it is a simple change written by someone with review rights.)</p><p>Contributors should use the github reviewers section on the PR to request reviews. After a contributor has pushed a set of changes in response to a review, they should refresh the github review status, so the users are notified it is ready for re-review. When the review is complete, a person with responsibility for merging changes to that part of the system should be added as a reviewer (or refreshed), with a comment that it is ready to merge.</p><p>Reviewers should check for PRs that are ready for their review via github&#39;s webpage (filter &quot;review-requested:&lt;reviewer-id&gt;&quot;) or via the github CLI (e.g. gh pr status). Contributors should similarly ensure they stay up to date with any comments on requests for change on their submissions.</p><h2 id="target-branch" tabindex="-1">Target branch <a class="header-anchor" href="#target-branch" aria-label="Permalink to &quot;Target branch&quot;">​</a></h2><p>The <a href="./VersionSupport.html">Version support</a> document contains details of the different versions that are supported, and which version should be targetted for different kinds of changes. Occasionally earlier branches will be chosen, (e.g. security fixes to even older versions) but they should always be carefully discussed (and documented).</p><p>Changes will always be upmerged into the next point release for all the more recent major and minor versions (and master).</p>',12),a=[r];function n(h,l,d,c,u,m){return i(),t("div",null,a)}const p=e(s,[["render",n]]);export{g as __pageData,p as default};
