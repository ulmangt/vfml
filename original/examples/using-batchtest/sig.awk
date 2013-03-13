BEGIN { first = 0; second = 0; firstSig = 0; secondSig = 0; }
$2 < $4 { first = first + 1 }
$4 < $2 { second = second + 1 }
($2 - $4) > ($3 + $5) { secondSig = secondSig + 1; print "second won big on", $1 }
($4 - $2) > ($3 + $5) { firstSig = firstSig + 1; print "first won big on", $1 }
$2 == $4 { print "Tie on ", $1 }

END { print "First won ", first, " -- ", firstSig, " by the sum of the stdevs" }
END { print "Second won ", second, " -- ", secondSig, " by the sum of the stdevs" }

