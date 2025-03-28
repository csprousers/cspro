# Report Title

<?
// To start executing logic within a report, write < immediately followed by ?.
// To end the section of logic, write ? immediately followed by >.
//
// There are three ways of writing to CSPro logic-based reports:
//
//     1. ~~...~~ writes the results of the numeric or string expression specified between the tildes.
//
//     2. ~~~...~~~ is similar to the above version except that the text will not be automatically escaped for Markdown.
//
//     3. $.write("...", ...); writes directly to the report. The text will not be escaped for Markdown.
//        The $.write function uses the syntax of the errmsg/maketext family of functions.
//
// In the Markdown variant that CSPro uses, ~~ can be used for strikethrough and ~~~ can be used for
// fenced code blocks. These tilde sequences are used for CSPro fills, so if you want to use these
// features in your report, you can use a trick, shown below, to insert these into your evaluated
// Markdown. You can also use ``` instead of ~~~ for code.
?>

### Templated Report Example

The current date and time is: **~~timestring()~~**. The year, **~~sysdate("YYYY")~~**, is
<?
    // Write out an italicised "not" if this is not a leap year:
    numeric current_year = sysdate("YYYY");

    if current_year % 4 <> 0 or ( current_year % 100 = 0 and current_year % 400 <> 0 ) then
        $.write("*not*");
    endif;

?> a leap year.


### Strikethrough

If you want to strikethrough, you can:

1. Use the &lt;del&gt; tag: <del>this is striken</del>
2. Wrap the text in a triple-tilde escape: ~~~"~~"~~~this is striken~~~"~~"~~~
