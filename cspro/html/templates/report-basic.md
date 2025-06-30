# Report Title

<?
// To start executing logic within a report, write < immediately followed by ?.
// To end the section of logic, write ? immediately followed by >.
//
// There are three ways of writing to CSPro text templates:
//
//     1. ~~...~~ writes the results of the expression specified between the tildes.
//
//     2. ~~~...~~~ is similar to the above version except the text will not be automatically encoded for Markdown.
//
//     3. $.write("...", ...); writes directly to the report. The text will not be encoded for Markdown.
//        The $.write function uses the syntax of the errmsg/maketext family of functions.
//        There are also the functions: $.writeLine, $.writeEncoded, and $.writeEncodedLine.
//
// In the Markdown variant that CSPro uses, ~~ can be used for strikethrough and ~~~ can be used for
// fenced code blocks. These tilde sequences are used for CSPro expressions, so if you want to use
// these features in your report, you can use a trick, shown below, to insert these into your
// evaluated Markdown. You can also use ``` instead of ~~~ for code.
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

1. Use the \<del\> tag: <del>this is striken</del>
2. Wrap the text in a triple-tilde template expression: ~~~"~~"~~~this is striken~~~"~~"~~~
