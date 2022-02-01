$startPath = ".\output\$args"
$magicWord = 'global'
foreach($file in Get-ChildItem $startPath -File)
{
    if($file.Name -match $magicWord)
    {
        Move-Item ".\output\$args\$file" .\output\$args\output_globale
    }
}