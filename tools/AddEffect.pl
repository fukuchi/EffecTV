#!/usr/bin/perl

# Perl script to add a new effect to EffecTV
# Usage: AddEffect.pl EffectName <template>
# This will make effects/EffectName.c, based on template.c (if specified)
# Otherwise it uses dumb.c as the base

# Make sure the file doesn't already exist
if ($#ARGV < 0) {
  print "Usage: AddEffect.pl EffectName template\n";
  print "Example: AddEffect.pl Spastic dumb\n";
  print "Will add SpasticTV (Spastic.c), using dumb.c for the code\n";
  exit(0);
}
chomp $ARGV[0];
$effect = $ARGV[0];
$uppereffect = uc $effect;
$effectname = $effect . "TV";
$filename = $effect . ".c";
if ($#ARGV == 0) {
  $template = dumb;
} else {
  $template = $ARGV[1];
}

# copy template, substituting appropriately....
open(TEMPLATE,"<effects/$template.c") || die "Couldn't open $_: $!\n";
@TEMPLATE = <TEMPLATE>;
close TEMPLATE;
open(OUT,">effects/$filename");

foreach $_ (@TEMPLATE) {
  s/$template/$effect/i;
  print OUT;  
}
close OUT;
open(MAKE,"<effects/Makefile");
@MAKE=<MAKE>;
close MAKE;
open (MAKE,">effects/Makefile");
foreach $_ (@MAKE) {
 if (m/^EFFECTS \=/) {
	s/\\/\$\($uppereffect\) \\/;
 }
 if (m/$template\.o/) {
	s/\n/\n$uppereffect \= $effect\.o $effect\.so\n/;
 } 
 print MAKE;
}
#open(H,">>effects/effects.h") || die "Can't open: $!\n";
#print H "extern effectRegistFunc " . $effect . "Register;\n";
#close H;

#open(MAIN,"<main.c") || die "dead\n";
#@MAIN = <MAIN>;
#open(MAIN,">main.c") || die "dead\n";

#foreach $_ (@MAIN) {
#  if (m/Register$/) {
# 	chomp $_;
#	print MAIN $_ . ",\n" . $effect . "Register\n";
#  } else {
# print MAIN;
#  }
#}
