# Theory behind ColorChord's fast DFT.

Becaue
1) ColorChord's binning process needs to focus on notes in frequency
 space instead of the bins a normal FFT produces, we have to use another
 tactic to decompose incoming audio into chromatic bins instead of normal
 bins.
2) We want bins which can be foldend on top of each other, i.e. each octave
 can add to the previous.
3) Originally ColorChord accomplished this by using a GPU algorithm which
 every frame looked at the last several milliseconds of sound and by brute
 force multipled a sine and cosine for every bin against the incoming audio
 with a window.  This was prohibitively expensive.
4) One night I think around 2010 or 2011, Will Murnane, Charles Lohr's
 roommate at the time had a pretty deep insight.  Charles is not good at
 math or algorithms, but Will is.
5) Much later on May 13, 2020, CaiB wanted to understand the process more,
 and Will Murnane explained this in the ColorChord discord channel.

This following is not an exact transcript but has minor edits for flow,
grammar, and clarity.

Will: Hey @CaiB I helped Charles figure out the DFT-ish thing that's implemented in colorchord.  I did the conceptual stuff, he wrote the actual code.

CaiB: Ooh, nice to meet you o/

Will: Basically the idea is this: if we can do something clever to get the note intensities within one octave, then we can average adjacent samples together as a simple low-pass filter, and repeat the clever thing to get notes an octave lower. So then the next insight is that we're looking for repeating patterns, and they can all be decomposed into sine waves. The reasoning behind this is complicated if you haven't seen it, but it's called fourier analysis, and there's a lot of tutorials around on the internet.

CaiB: I know enough about this stuff to kinda get the gist, but not enough to be practically useful in comprehensive understanding or implementation.

Will: So, okay, sine waves. let's start with a simple idea: suppose we just guess that the frequency is exactly some value. that is, our samples are `f(t) = sin(t*omega)` for some value of `omega`. So if we multiply our samples by `sin(t*omega)` again, we get `sin(t*omega)*sin(t*omega)` also notated `sin^2(t*omega)`

CaiB: OK, yeah (If it's easier for you, we can do a voice call so you don't have to type a bunch of stuff)

Charles: I would kind of like to document the text.

Will: The way I think of this is basically: if we happen to pick the right value of omega, those values will all tend to add up faster than if we pick the wrong value
(really, it's more like `sin(t*omega_actual) * sin(t*omega_guessed))`)

CaiB: OK, so kinda like constructive vs destructive interference but multiplied not added.

Will: Exactly! the intuition is that if we pick `omega_guessed` to be nearly equal to `omega_actual` the sum over the window we're considering will add up to a bigger value than if we pick a guess that's, say, 10% too big.

CaiB: Gotcha

Will: The next layer to add is this: We're assuming everything is gonna get decomposed into sine waves. However, a sine wave isn't just parameterized by a frequency, there's also phase information. So to account for this potential phase shift, we can multiply our signal with two functions with a 90 degree phase shift
two functions that happen to have a 90 degree phase shift are the sine and the cosine.
CaiB: Oooh, that way you can look at proprtions of the outputs to get an idea of the actual phase.

Will: So if we take `f(t) = sin(omega*t + phi)` and then do `f(t) * sin(omega * t) + f(t) * cos(omega*t)` then regardless of the value of phi the sum over our window adds up to the same value! You could extract phase information, but colorchord doesn't actually do that iirc.
CaiB: When you say "over the window", what kind of ballpark are we talking? 10 samples? 100?

Will: Usually it makes sense to think of the window as being in seconds rather than samples. Your sample rate is basically totally up to you, but humans perceive a fixed frequency range around 20hz-20khz

CaiB: I mean, for ColorChord it's just set by the system audio mode, so around 44.1k-48k usually..

Will: Yeah, but you can do it on a microcontroller like an AVR where you're doing the ADC yourself at some arbitrary rate and the window can be a sliding window, in the sense that you don't need to process entirely non-overlapping blocks. You could even get to doing this once per sample, just using the last N samples each time.
CaiB: Now if I'm remembering correctly, your window size significantly impacts what resolution your output is?

Will: The window size basically only determines the speed of reaction to new inputs. If you used a 10 second window, you wouldn't notice a new note until it had been around for a few seconds for example ok, so. we figured out how to determine how much a single frequency/note is happening in a window of time. But there's still a bunch of FP math going on, and depending on what system you're dealing with you might have a slow FPU or no sin/cos function in hardware and getting only the intensity of a single frequency doesn't give us very much pitch information at all. so how can we get more? make more guesses in parallel!

Will: In 12-tone just intonation, the rule is that each half-step is `2^(1/12) ~= 1.06` times higher than its neighbor. so within each octave, we have a separate data structure for the intensity of each half-step so we get 12 of these, with frequencies `[1, 1.06, 1.12, 1.19, 1.26, 1.33, 1.31, 1.50, ...]`
CaiB: Let's say you're guessing at 100 Hz and 110 Hz, both of them being let's say 50% strong. How do you know the difference between a sound signal that just has weak content at 100 and 110 vs strong content at 105?

Will: We'll get there... But ultimately the answer is that you can do curve fitting on the values you get out. you're right, feeding 105 hz into a thing with 100 and 110 hz guesses will give strong values for both (it's actually an exponential system, so the "halfway" point is `sqrt(100*110) = 104.88`, not `105`, but the point stands)
but we can also look at the adjacent bins even further outside to give an indication of where the peak is. suppose we're dealing with a `108 hz` signal, and we have bins at `90, 100, 110, 120`.
CaiB: Aah.

Will: We can fit a curve to the intensities in those bins, and we'll find a peak at approximately `108hz`. so as long as we have a decent number of bins, the location of the peak should be pretty easy to find.
CaiB: Because one at `108` is a sharper peak (less spread to further bins) than two at `100` and `110`.

Will: (so ultimately, the exact values of the bins don't really matter, but it's easier for me to think of it as a 12-tone scale) well... there isn't a bin at 108hz, only one at `100` and `110`. Usually music tends to have harmonic content, so you don't see `100hz` played next to `110hz`, you see a single `108hz` note. so colorchord basically assumes you're playing music and  you would prefer the answer `108hz`.

CaiB: Sorry, I meant one peak in frequency content at `108hz` would mean bins `90hz` and `120hz` see lower numbers than an input with content at `100hz` and `110hz`, where `90hz` and `120hz` would see larger numbers.

Editorial Note from Charles: The section below is an extremely powerful concept.  It provides a significant speedup from needing a GPU to being able to run on a CPU. That and realizing we could progressively update.

Will: Yeah, the spread of the distribution is important. And if you added more bins, you'd be able to see the distinction between `100hz+110hz` and `108hz` alone the expensive part of adding more resolution is adding more bins within a single octave, but there's a clever thing we can do to get all the octaves we can hear for only 2x the cost of doing a single octave.  Effectively all of our base frequency bins are in the highest octave we care about. then by doing the 2x averaging i mentioned earlier, we get one octave lower. And by repeatedly averaging samples, we get additional octaves lower basically, think of it like this: 
* Every sample we calculate octave 8.
* Every 2nd sample we average the last 2 samples from octave 8, and use them to calculate octave 7.
* Every 4th sample we average the last 2 samples from octave 7, and use them to calculate octave 6.
* 8th, 2, 6, 5
* 16th, 2, 5, 4
* ...
By cleverly interleaving the work, we can do a fixed amount of averaging-and-higher-octave-stuff every tick the octave we work on at each tick is determined by the number of zeroes at the end of the binary representation of the tick number.
`0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, ...`

CaiB: Neat. I'm still not quite understanding how the 2x averaging works though, you'd still need to compare it to a sine wave at the lower frequency, right?

SamE: Question: Why not use the aliasing property of the DFT to get the other frequency bins?

Will: By averaging, we're effectively getting samples that count for twice as much time, or half as much frequency, which means notes exactly one octave lower.

Will: @SamE: I may well not understand it well enough to take advantage of it correctly, but let's talk it out a bit.

SamE: TBH: I don't understand it either.

Will: @CaiB fyi here's [the definition of the DFT on wikipedia](https://en.wikipedia.org/wiki/Discrete_Fourier_transform#Definition) so you can see how the formalism correlates to the extremely-informal construction I gave above.

Will: Is there good math formatting maybe? `$f(x) = 3x$`

CaiB: Yeah, that just flies right over my head by the time I get a few sentences in.  Haha, Discord is not fancy enough for anything other than the most basic of formatting.

Will: The second part with the sines and the cosines is basically what I was talking about. @Sam E so my understanding is basically that because of aliasing, frequencies higher than your sample rate wind up reflected off your sample rate.  Which we avoid by using averaging as a low pass filter, so that there can be no frequencies higher than sample rate. iow (?) the highest octave is directly from captured audio samples, so it doesn't contain any information higher than nyquist allows. each of the lower octaves is downsampled to remove one octave off the top.

CaiB: OH, you're taking the average of every two samples, then treating that as the _same_ sample rate.

Will: Yes. you want to only keep half as many samples around, though, or you'll be considering twice as long a window in real-time.

CaiB: Essentially speeding up the audio by 2x, then comparing it to the same standard sine waves.

Will: Yep!

CaiB: Ooooh. I was thinking you were taking a 48kHz sample rate signal, averaging them and treating it as a 24kHz signal, which would just get you a worse version of the same result.

Will: Whoa, here's a picture from wikipedia that describes things very similar to what i'm talking about! ![WOLA Channelizer](https://en.wikipedia.org/wiki/Filter_bank#/media/File:WOLA_channelizer_example.png)

CaiB: And that way you can precompute the comparisons, one sine and one cosine for each bin in only one octave?

Will: Yeah! the other trick you can play is to just add the samples instead of averaging. this means you take (let's say) two 16-bit samples, and get one 17-bit result which is twice as large on average as a highest-octave 16-bit sample but you only have half as many of those 17-bit samples in your window, and by definition their sum is the same as the sum of the 16-bit samples so the magnitudes you get per octave are comparable.

CaiB: Then divide the output by two. Half the resolution, but about half the calculations. Oh!

Will: In other words you don't get a reading that says "note 6 in octave 3 is twice as loud as in octave 4" when in fact they're equal.

SamE: What I was saying is trying to be clever (always dangerous) and try and use the aliasing to stack all of the information we want into the same bins for us. One would probably have to add in a few more DSP filters in order to flip some of the higher frequency images. I might have to experiment with this later.

CaiB: So on the output, you're updating the top octave every sample, the second-highest octave every other sample, etc, until the lowest is only updating once in a while (comparatively)? And that still works OK?

Will: Using some kind of clever sub-sampling to fold things around might work: [Alias wikipedia page, section Folding](https://en.wikipedia.org/wiki/Aliasing#Folding) Yeah, if you think about it the bottom octave can't change too fast or it wouldn't be in the bottom octave anymore :open_mouth:

CaiB: But if a low tone starts and stops playing repeatedly, you may end up completely losing that info, and just seeing a weaker signal instead of the actual pulsing.

Will: Take a pathological example: consider a wave where the top half of each pulse is at 40 hz, and the bottom is at 38hz wellllll... that's really just a 39hz wave with some distortion!

CaiB: I'm thinking more along the lines of take a 40Hz sine signal, then multiply it by a 0-1 square wave at 200 Hz. You'd just see the 40Hz at a intensity corresponding to the duty cycle of the square wave, not the actual on-off pattern.

Will: Well, you'd see an output at 40hz corresponding to the amount of time that the 40hz signal is present for sure. but you'd also see a strong 200hz (and many higher-frequency!) signals corresponding to the 200hz square wave

CaiB: But you have no way of knowing that the 40Hz signal is actually starting and stopping vs just being weaker.

Will: a description of how you got to a particular waveform isn't the same as a description of what it sounds like :wink:

SamE: In the frequency domain, aren't they essentially the same thing.

Will: And if you want a system that changes its fundamental answer 400 times a second, you probably do not want an audio system.

CaiB: @SamE, My point is if you were to just watch a live graph of the frequency domain, you'd see the low frequencies refresh slowly, and won't be able to see a pulsing signal down there as actually pulsing.  Whereas a 5kHz signal turning on and off at that same 200Hz you could clearly see turning on and off because your graph refreshes fast enough up there.

Will: I think if you wanted to see things at that level you could, by subtracting out old samples from that octave and adding in new ones live... but i don't think that audio sounds like you think.

SamE: Okay, I need to turn this off so that I can finish a presentation for work. Thanks for explain this to us mere mortals. :slight_smile:

CaiB: OK, it probably wouldn't be useful to audio but for actually analyzing a signal thoroughly it may be a problem.  But I guess that's outside the scope I might try to implement this from scratch when I have some free time, since trying to look at the existing code just makes my brain stop working. So let's say your window is 1000 samples long. You're doing `1000 samples * (1 for sin + 1 for cos) * 24 bins` number of calculations every single sample, just to get the highest octave? Also, is this something you came up with yourself? If so, how? :exploding_head:

Will: Example Audio (chop.wav) / (chop.pv).

CaiB: Hmm, how is the actual sin/cos multiplication step done? Because if you just go in with no previous info, (assuming 1000 sample window) you'd need to multiply every sample by the precomputed sin and cos, so 2000 multiplications. Then, assuming you don't want phase info, you'd maybe just add each sin and cos value together, but then you also need to sum all of them up in order to compare the entire window, so 1000 adds. And now do that for every bin, so 24 times just for that one octave. That's a lot of math.

Will: there are a bunch of tricks you can play to not do so much work. for example, instead of using processing you could use more memory: you can remember the results of `sin*sample + cos*sample` and then just subtract the sample that's falling out of your window and add the one that's coming in. Or approximate your windowing function using an IIR filter: just multiply your "last" value by something like 0.999 each sample, and the influence of old samples will be forgotten implicitly over time.

CaiB: Charles does love his IIRs (assuming he's the one that made the post-DFT analysis).

Editorial Note: ColorChord almost exclusively uses IIRs for this purpose. In practice they work/feel better than other triangular solutions because of the logarithmic nature of how we perceive sound.  Also, yes, Charles rarely works on a project more than a month before he finds a use for IIRs.

Will: I built the description in my head of why this works, but i did so by reading a lot on DFT and DTFT and taking a class on complex analysis. stay in school, i guess? Hah, so he does.

CaiB: "school" did a great job of making me hate every subject I took, even ones I used to love previously...

Will: PS: the audio file above is generated using the method you mentioned above. i think it shows that mostly what you hear is 200hz and harmonics of that.

CaiB: Not quite, your square wave is -1 to 1, but I get the point.

Will: yeah, I agree that school can be a backwards way of approaching subjects if you can become interested in the subject first, and then learn the formality behind it, it can be a great way of figuring out how to apply things, but if you have to slog through the formality with no idea where you're going it's not exactly gonna instill a love of the subject. Yup, so it is, change the definition of `c` to be all that stuff `+1` and it'll be what you were talking about. Doesn't sound much different though.  (see chop2.wav)

![chopfast.png](https://raw.githubusercontent.com/cnlohr/colorchord/master/docs/TheoryOfCCDFT/chopfast.png)

CaiB: It's more just that the methods taught are often so abstract it feels pointless. An entire class I took was just doing RLC-type circuit analysis using various methods like Laplace transforms. When in the real world, you just plug it into a calculator and will never use that unless you are the 0.1% of people who develop those calculators or other weird applications... 40 and 200 were a bad choice, speeding it up by 4x, you can definitely hear both tones.  (See chopfast.wav)

Will: Sure, but my point earlier is that colorchord will probably tell you "this is mostly 200hz content, with some 40" because that's what it sounds like.

CaiB: Yeah, good enough.

Will: Not "this is 40hz, now nothing, now 40hz, now nothing..."

CaiB: I mean, by the time it shows up, 200Hz changes would be completely smoothed to oblivion anyways. It's just steady this. ![cc-screenshot3.png](https://raw.githubusercontent.com/cnlohr/colorchord/master/docs/TheoryOfCCDFT/cc-screenshot3.png)

Will: Yeah, what's happening in colorchord is that it squashes all the octaves together.

CaiB: Yeah, I understand all the processing happening after the DFT. Just a couple of days ago I went through line-by-line, commented and rewrote some parts to actually understand what it does.

Will: So it sees 40hz, and 200/4 = 50hz, which is a perfect major third: (Wikipedia article on Major Thirds)[https://en.wikipedia.org/wiki/Major_third] so it makes green for one of those and red for the other.

CaiB: E1-ish = 40Hz, G1-ish = 50Hz. Mine is looking at the 4x version because ColorChord doesn't even look below 55Hz, so more like E3ish and G3ish. Also because my speakers can't output 40Hz enough to be useful here, so I 4xed it to hear the low tone.

Will: yeah, eventually you need a cutoff on how low you look, aka how many times you average pairs of samples. Also, depending on what shape of sine wave you pick for the highest rate, you can limit how high you listen but still get good resolution and you don't even necessarily need to use many samples, or good-quality samples for your sin/cos tables! Like your high-res sin table, in full, could be: `[0, 1, 0, -1]` And the one at the other end of the octave could be `[1, -1, 1, -1]` or so.

CaiB: So assuming 5 octaves like is used, and no clever interleaving, every `2 << (5 - 1) = 32` samples you need to calculate all octaves

Will: Yeah, but over those 32 samples you need to do: highest octave: 32 calculations next-highest: `16, 8, 4, 2, 1` total: 63 calculations (and 31 sums/averages)

CaiB: Wouldn't it be 63 * 2 because `sin` and `cos`.

Will: Well, sure, for some complicated version of "calculations"

CaiB: OK, fair.

Will: And you need to do some adds and so on.  This is all handwavy.

CaiB: Hold up, 4 element tables? I'm amazed you don't just get literal garbage out of that.

Will: well it's not great, that's for sure!

CaiB: Looks like the table in the actual implementation is 256 long.

Will: Since you're aligning a longer sequence to it, you wind up re-using elements often enough to make it kinda work buuut don't start with a 4-element table as a goal. definitely a distorted sine wave.

CaiB: Yeah, like said when I have time, I'm going to try to recreate it from scratch, now that I can actually see the logic behind it. A decent while back I ported ColorChord to C#, and copy-pasted/re-wrote everything but the DFT because it looks so alien and scary, so in my fork it literally is still the original C code sitting there all alone. Well thank you very much for taking the time to explain this, very much appreciated

Will: Happy to help! hope your reimplementation goes well.
