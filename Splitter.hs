-- 
-- Copyright © 2007, The Board of Trustees of the University of Illinois.
-- 
-- This file is part of SSNiper.
-- 
-- SSNiper is open source software, released under the University of
-- Illinois/NCSA Open Source License.  You should have received a copy of
-- this license in a file with the SSNiper distribution.
-- 
-- ----------------------------------------------------------------------
--  Author:  Joshua Stone
--  Contact: joshs@uiuc.edu
--  Date:    Nov 27, 2007
-- ----------------------------------------------------------------------
-- 
-- Usage:
-- 
-- You should compile this with a recent GHC, as follows:
-- 
-- $ ghc --make -o splitter -O3 Splitter.hs 
-- 
-- Then you can run it on a result file.  Suppose that you want to group
-- results by the 2nd field in the path name (i.e., like home directories 
-- in /home: /home/foo, /home/bar, /home/baz, etc.):
-- 
-- $ ./splitter ssniper_results.log 2 homedirs
-- 
-- This will create a directory called "homedirs" in the current working
-- directory and output a file for each group identified with some 
-- additionaly filtering of filenames and categorization of results.
-- 

module Main where

import Directory
import System
import List
import IO
import Monad
import Data.HashTable as HT
import Maybe
import Text.Printf

ratioCutoff = 40
hn          = 50


-- tell everyone who we are and where we came from
banner :: IO ()
banner = do putStrLn "SSNiper Result Splitter (maintainer: Joshua Stone, joshs@uiuc.edu)"
            putStrLn "Copyright (C) 2007 The Board of Trustees of the University of Illinois"
            putStrLn "----------------------------------------------------------------------"
            putStrLn "(License: University of Illinois/NCSA Open Source License)"
            putStrLn ""


-- load results, filter them down, group them, and write the subreports
main = do args <- System.getArgs
          banner
          if (length args) /= 3
             then do putStrLn "  usage: splitter <infile> <path-name-field> <dir>"
                     putStrLn "\n     Files will be placed in ./<dir>/<name>.slog\n"
             else do putStrLn("Reading results from " ++ (args!!0))
                     hits <- loadFile (args!!0)
                     let filtered = stripFalses hits
                     hash <- buildHash filtered ((read (args!!1))::Int)
                     list <- HT.toList hash
                     putStrLn ("Found " ++ (show $ length list) ++ " subdirs with interesting hits")
                     putStrLn ("Creating directory '" ++ (args!!2) ++ "'")
                     catch (Directory.createDirectory (args!!2)) 
                               (\x -> do putStrLn("  Already exists!  Proceeding..."))
                     putStrLn("Writing individual subfiles")
                     mapM (writeSubFile (args!!2)) list
                     return ()


-- names of the categories of results
groups = ["High Risk of Both Delimited And Non_Delimited SSNs:",
          "High Risk of Delimited SSNs:"                       ,
          "High Risk of Non-Delimited SSNs:"                   ,
          "Low Risk of Delimited SSNs:"                        ,
          "Low Risk of Both Delimited And Non_Delimited SSNs:" ,
          "Low Risk of Non_Delimited SSNs:"]


-- remove results that are very likely false positives
stripFalses :: [Result] -> [Result]
stripFalses res = nameReduced
    where ratioReduced = filter (\x -> (ratio x) > ratioCutoff) res
          nameReduced  = filter goodName ratioReduced


-- strip out obviously false positives (base on filename)
goodName :: Result -> Bool
goodName res = not (foldr ((||).matchIt) False badness)
    where badness = [".mozilla",           -- Mozilla profile dirs (caches, etc.)
                     "Firefox/Profiles",   -- Mozilla profile dirs on Windows
                     "/."]                 -- Hidden files / directories
          matchIt a = subList a (file res)


-- data type to describe an SSNiper result "hit"
data Result = Result { delim :: Int, nondelim :: Int,
                       total :: Int, falses   :: Int,
                       ratio :: Int, file     :: [Char] }


-- categorize results based on their numerical attributes
categorizeHits :: [Result] -> [[Result]]
categorizeHits [] = [[],[],[],[],[],[]]
categorizeHits (x:xs)
    | d >   0 && n >   0 && (d+n) > 20 = pushNth x 0 (categorizeHits xs)
    | d >  10 && n ==  0               = pushNth x 1 (categorizeHits xs) 
    | d ==  0 && n >  hn               = pushNth x 2 (categorizeHits xs) 
    | d >   0 && n >   0               = pushNth x 3 (categorizeHits xs) 
    | d >   0 && n ==  0               = pushNth x 4 (categorizeHits xs) 
    | d ==  0 && n >   0               = pushNth x 5 (categorizeHits xs) 
    where { d = delim x ; n = nondelim x }


-- take a line and return a Result object 
parseResult :: [Char] -> Result
parseResult line = Result (n!!0) (n!!2) (n!!4) (n!!6) (n!!8) ((tail.tail.tail.snd) sides)
    where n      = map (\x -> (read x)::Int) ((words.fst) sides)
          sides  = break (=='%') line


-- take a list of lines and "parse" them to Result objects
parseResults :: [[Char]] -> [Result]
parseResults []     = []
parseResults (l:ls) = (parseResult l):(parseResults ls)


-- open a file of results from SSNiper and return them as parsed 'Result's
loadFile :: [Char] -> IO [Result]
loadFile file = do contents <- lines `liftM` readFile file
                   return (parseResults contents)


-- look through hits and group them by a field in the path
buildHash :: [Result] -> Int -> IO (HT.HashTable String [Result])
buildHash lst n = do a <- HT.new (==) hashString
                     mapM (\x -> push a (calcField (file x) n) x) lst
                     return a
    where push h k v = do ov <- HT.lookup h k
                          let rest = fromMaybe [] ov
                          HT.update h k (v:rest)


-- for each group, write out a report summary in the provided dir
writeSubFile :: [Char] -> ([Char], [Result]) -> IO ()
writeSubFile dir (name, res) =
    do f <- openFile (dir ++ "/" ++ name ++ ".slog") WriteMode 
       writeCats f groups (categorizeHits res)
       hClose f


-- write SSN categories out to a provided handle (presumably a file)
writeCats :: Handle -> [[Char]] -> [[Result]] -> IO ()
writeCats h group [] = do return ()
writeCats h (g:gs) ([]:rs) = do writeCats h gs rs
writeCats h (g:gs) (r:rs) =
    do hPutStrLn h ("\n" ++ g)
       mapM (printRes h) (sortBy cmpRes r)
       writeCats h gs rs
    where printRes h (Result _ _ t _ _ p) = hPrintf h "%6d %s\n" t p
          cmpRes a b = (flip compare) (total a) (total b)


-----------------------
-- UTILITY FUNCTIONS --
-----------------------


-- return the nth element when filename is split on '/' chars
calcField :: [Char] -> Int -> [Char]
calcField filename n = take len (drop off filename)
    where indices   = findIndices (=='/') filename
          off       = (indices!!n) + 1
          len       = (indices!!(n+1)) - (indices!!n) - 1


-- true if a is a sublist of b (think substring)
subList :: (Eq a) => [a] -> [a] -> Bool
subList a []                   = False
subList a b | a `isPrefixOf` b = True
subList a (_:bs)               = subList a bs


-- given a list of lists, push n onto the pth sublist
pushNth :: a -> Int -> [[a]] -> [[a]]
pushNth n 0 (a:as) = (n:a):as
pushNth n p (a:as) = a:(pushNth n (p - 1) as)


