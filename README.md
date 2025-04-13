# Regex parser

The idea with this project is to experiment with regex parsing!

I want to be able to take both an input string and a regular expression and match them. I probably want to be able to point out any matching substrings in the input.
I'm following a textbook on compiler construction and also a guide on regex parsing. Specifically I'm using my old university textbook 'Introduction to Compiler Design by Torben Mogensen' and loosely following this article on regex matching: https://swtch.com/~rsc/regexp/regexp1.html

As of right now I have done:

- ✅ Converting regex to postfix notation for easier conversion to an NFA
- (In progress) NFA generation
- ❌ NFA to DFA conversion
- ❌ Substring matching
