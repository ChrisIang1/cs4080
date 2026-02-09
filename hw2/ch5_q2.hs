class ExprSYM repr where
  lit :: Int -> repr
  add :: repr -> repr -> repr

newtype Eval = Eval { runEval :: Int }

instance ExprSYM Eval where
  lit n     = Eval n
  add a b   = Eval (runEval a + runEval b)

newtype Pretty = Pretty { runPretty :: String }

instance ExprSYM Pretty where
  lit n   = Pretty (show n)
  add a b = Pretty ("(" ++ runPretty a ++ " + " ++ runPretty b ++ ")")

prog :: ExprSYM repr => repr
prog = add (lit 1) (lit 2)

main :: IO ()
main = do
  putStrLn ("Eval: " ++ show (runEval (prog :: Eval)))
  putStrLn ("Pretty: " ++ runPretty (prog :: Pretty))